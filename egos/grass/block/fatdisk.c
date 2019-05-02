#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "grass.h"
#include "block_store.h"
//#ifdef HW_FS

#include "fatdisk.h"
void panic(const char *s);

/* Temporary information about the file system and a particular inode.
 * Convenient for all operations. See "fatdisk.h" for field details.
 */
struct fatdisk_snapshot {
    union fatdisk_block superblock;
    union fatdisk_block inodeblock;
    block_no inode_blockno;
    unsigned int inode_no;
    struct fatdisk_inode *inode;
};
static block_t null_block;
struct fatdisk_state {
    block_store_t *below;   // block store below
    unsigned int inode_no;  // inode number in file
};
static block_no fatblocks;
/**
 * This function map fat entry in fat block into datablock that this entry corresponds to
 * @param n fatentry index
 * @param below block stored below
 * @return datablock index
 */
static block_no fatentry_to_datablock(fatentry_no n, block_store_t *below){
    union fatdisk_block superblock;


    if ((*below->read)(below, 0, (block_t *) &superblock) < 0) {

        return -1;

    }
 return superblock.superblock.n_inodeblocks + 1 + superblock.superblock.n_fatblocks + n;


}
/**
 * This function finds out this fat entry is in which block
 * @param below
 * @param fatentry
 * @return block number
 */
static block_no find_fat_blockno(block_store_t * below , fatentry_no fatentry) {
    union fatdisk_block superblock;

    if ((*below->read)(below, 0, (block_t *) &superblock) < 0) {
        return -1;

    }

    block_no bno = superblock.superblock.n_inodeblocks  + fatentry / FAT_PER_BLOCK + 1;
    return bno;

}
/**
 * This function finds a free entry(which next field = 0) among all fat entries
 * if a fat entry is in use, either its next field = -1 or another positive number
 * @param below
 * @param bno
 * @return
 */

static fatentry_no find_free_entry(block_store_t * below, block_no bno) {
    union fatdisk_block superblock;

    if ((*below->read)(below, 0, (block_t *) &superblock) < 0) {
        return -1;
    }
    unsigned int nfatblocks = fatblocks;
    /**
     * start to search from the current fat_free_list all the way till the end of fatblocks
     */
    unsigned int i = superblock.superblock.fat_free_list % FAT_PER_BLOCK ;
    while(nfatblocks--) {
        union fatdisk_block fatblock;
        if((*below->read)(below, bno, (block_t *) &fatblock) < 0) {
            panic("fatdisk: Find free entry failed\n");
            return -1;
        }
        for(; i < FAT_PER_BLOCK; i++) {
            fatentry_no next = fatblock.fatblock.entries[i].next;
            if(next == 0) {
                // unused has found
                fatentry_no  res = (bno - 1 - superblock.superblock.n_inodeblocks) * FAT_PER_BLOCK + i;
                return res;
            }

        }
        bno++;
        i = 0;
        if(bno >= 1 + superblock.superblock.n_inodeblocks + fatblocks)
            bno = 1 + superblock.superblock.n_inodeblocks;
    }
    //Did not find a free entry
    panic("fatdisk_find_free_entry: The disk is full\n");
    return -1;


}
/**
 * This function allocates a free data block and set its corresponsing fat entry's next to -1
 * to indicate this data block has been occupied
 * @param below
 * @param snapshot
 * @return this datablock's fatentry
 */
static fatentry_no fatdisk_alloc(block_store_t *below, struct fatdisk_snapshot * snapshot) {
    fatentry_no fidx = snapshot->superblock.superblock.fat_free_list;
    block_no fidx_bno = find_fat_blockno(below, fidx);

    block_no free_blockno = fidx;
    struct fatdisk_fatblock fatblock;

    if ((*below->read)(below, fidx_bno, (block_t *) &fatblock) < 0) {
        panic("fatdisk_alloc_block");
        return -1;
    }
    unsigned int idx = fidx % FAT_PER_BLOCK;
    fatblock.entries[idx].next = -1;

    if ((*below->write)(below, fidx_bno,(block_t *) &fatblock) < 0) {
        panic("fatdisk_alloc_block: write fat block");
        return -1;
    }
    snapshot->superblock.superblock.fat_free_list = find_free_entry(below, fidx_bno);

    if ((*below->write)(below, 0, (block_t *) &snapshot->superblock) < 0) {
        panic("fatdisk_alloc_block: freelistblock");
    }
    return free_blockno;

}


static int fatdisk_get_snapshot(struct fatdisk_snapshot *snapshot,
                                block_store_t *below, unsigned int inode_no) {
    snapshot->inode_no = inode_no;

    /* Get the super block.
     */

    if ((*below->read)(below, 0, (block_t *) &snapshot->superblock) < 0) {
        return -1;
    }

    /* Check the inode number.
     */
    if (inode_no >= snapshot->superblock.superblock.n_inodeblocks * INODES_PER_BLOCK) {
        fprintf(stderr, "!!TDERR: inode number too large %u %u\n", inode_no, snapshot->superblock.superblock.n_inodeblocks);
        return -1;
    }

    /* Find the inode.
    */
    snapshot->inode_blockno = 1 + inode_no / INODES_PER_BLOCK;
    if ((*below->read)(below, snapshot->inode_blockno, (block_t *) &snapshot->inodeblock) < 0) {
        return -1;
    }
    snapshot->inode = &(snapshot->inodeblock.inodeblock.inodes[inode_no % INODES_PER_BLOCK]);

    return 0;
}
/**
 * This function partitions the rest free blocks into fat entry blocks and data blocks
 * to make sure they have the same number, 1 fat entry per data block
 * @param below
 * @param next_free
 * @param nblocks
 * @return the first free fat entry
 */
block_no setup_fatfreelist(block_store_t *below, block_no next_free, block_no nblocks) {
    union fatdisk_block superblock;
    if ((*below->read)(below, 0, (block_t *) &superblock) < 0) {
        return -1;
    }
    block_no fatlist_data[FAT_PER_BLOCK];
    block_no fatlist_block = next_free;
    unsigned int i = 0;
    unsigned int n_fatblocks = 0;
    while(next_free < nblocks) {
        for(i = 0; i < FAT_PER_BLOCK && next_free < nblocks; i++) {
            fatlist_data[i] = 0;
            next_free++;
        }
        for(; i < FAT_PER_BLOCK; i++) {
            //the last fat entry block has no more data blocks to link, set it to -1
            fatlist_data[i] = -1;
        }
        next_free++;
        n_fatblocks++;
        if ((*below->write)(below, fatlist_block, (block_t *) &fatlist_data) < 0) {
            panic("fatdisk_setup_freelist");

        }
        fatlist_block++;
    }


    fatblocks = n_fatblocks;


    return 0;

}
/**
 * This function traverse underlying inode's file to its offset block
 * @param this_bs
 * @param offset
 * @return offset's block fat entry
 */
static fatentry_no fatdisk_traverse(block_store_t *this_bs, block_no offset) {

    struct fatdisk_state * ft = this_bs->state;

    struct fatdisk_snapshot snapshot;
    union fatdisk_block fatblock;
    if(fatdisk_get_snapshot(&snapshot, ft->below, ft->inode_no) < 0) return -1;


    if(offset >= snapshot.inode->nblocks) {
        fprintf(stderr, "!!TDERR: fatdisk_read offset too large\n");
        return -1;
    }
    block_no start = snapshot.inode->head;
    /**
     * start to traverse from the inode's head
     */
    while(offset--) {
        block_no fat_bno = find_fat_blockno(ft->below, start);

        if((*ft->below->read)(ft->below, fat_bno, (block_t *) &fatblock) < 0) return -1;

        fatentry_no index = start % FAT_PER_BLOCK;
        fatentry_no next = fatblock.fatblock.entries[index].next;
        start = next;
    }
    return start;



}
/* Create a new FAT file system on the block store below
 */
int fatdisk_create(block_store_t *below, unsigned int n_inodes) {
    /* Your code goes here:
     */
    unsigned int n_inodeblocks = (n_inodes + INODES_PER_BLOCK - 1) / INODES_PER_BLOCK;
    unsigned int nblocks = (*below->nblocks)(below);
    if(nblocks < n_inodeblocks + 2) {
        fprintf(stderr, "fatdisk_create: too few blocks\n");
        return -1;
    }
    union fatdisk_block superblock;
    if ((*below->read)(below, 0, (block_t *) &superblock) < 0) {
        return -1;
    }
    //initialize super block
    if(superblock.superblock.n_inodeblocks == 0) {
        // union fatdisk_block superblock;
        memset(&superblock, 0 ,BLOCK_SIZE);
        superblock.superblock.n_inodeblocks = n_inodeblocks;
        superblock.superblock.fat_free_list = setup_fatfreelist(below, n_inodeblocks + 1, nblocks);
        superblock.superblock.n_fatblocks = fatblocks;
        if ((*below->write)(below, 0, (block_t *) &superblock) < 0) {
            return -1;

        }
        //initialize inode blocks with zero content
        unsigned int i;
        for (i = 1; i <= n_inodeblocks; i++) {
            if ((*below->write)(below, i, &null_block) < 0) {
                return -1;
            }
        }
    } else {
        assert(superblock.superblock.n_inodeblocks == n_inodeblocks);
    }

    return 0;



}


static void fatdisk_free_file(struct fatdisk_snapshot *snapshot,
                                      block_store_t *below) {

    if(snapshot->inode->nblocks == 0) {
        return;
    }
    /**
     * Find the head fat entry of this inode
     */
    block_no nblks = snapshot->inode->nblocks;
    fatentry_no fatentry = snapshot->inode->head;
    block_no fatbno;
    union fatdisk_block fatblock;
    /**
     * Iteratively set the fat enrty's next to 0.(which I set to be unused flag)
     */
    while(nblks) {

        fatbno = find_fat_blockno(below, fatentry);
        printf("fatdisk_freefile: fatblock number is %d\n",fatbno);

        if((*below->read)(below, fatbno,(block_t*) &fatblock) < 0) {
            panic("fatdisk: fatdisk_freefile\n");
            return;
        }

        block_no target = fatentry_to_datablock(fatentry, below);
        char zeros[BLOCK_SIZE];
        memset(zeros, 0, sizeof(zeros));
        if ((*below->write)(below, target, (block_t*) zeros) < 0) {
            panic("treedisk_free_block: target block");
        }
        fatentry_no next = fatblock.fatblock.entries[fatentry % FAT_PER_BLOCK].next;
        fatblock.fatblock.entries[fatentry % FAT_PER_BLOCK].next = 0;
        if((*below->write)(below, fatbno,(block_t*) &fatblock) < 0) {
            panic("fatdisk: fatdisk_freefile\n");
            return;
        }
        fatentry = next;
        nblks--;

    }
    /**
     * Update the inode info to zero(unused flag)
     */
    snapshot->inode->nblocks = 0;
    snapshot->inode->head = 0;
    if((*below->write)(below, snapshot->inode_blockno,(block_t*) &snapshot->inodeblock) < 0) {
        panic("fatdisk: fatdisk_freefile\n");
        return;
    }

}


/* Write *block at the given block number 'offset'.
 */
static int fatdisk_write(block_store_t *this_bs, block_no offset, block_t *block) {

    /* Get info from underlying file system.
     */
    int dirty_inode = 0;

    struct fatdisk_state *fs = this_bs->state;
    struct fatdisk_snapshot snapshot;

    if (fatdisk_get_snapshot(&snapshot, fs->below, fs->inode_no) < 0) {
        return -1;
    }
    /**
     * If offset is bigger than the file size, allocate more blocks
     */
    int addblks = offset + 1 > snapshot.inode->nblocks ? (offset + 1 - snapshot.inode->nblocks) : 0;
    fatentry_no  last_entry = 0;
    if(snapshot.inode->nblocks > 0) {
        last_entry = fatdisk_traverse(this_bs, snapshot.inode->nblocks - 1);
    }
    /**
     * start to allocate blocks
     */
    while(addblks--) {
        union fatdisk_block lastfatblock;

        fatentry_no free_bno = fatdisk_alloc(fs->below, &snapshot);
        block_no last_entry_bno;
        if (snapshot.inode->nblocks == 0) {
            //this inode is new, set up its head
            snapshot.inode->head = free_bno;
            dirty_inode = 1;

        } else {
            last_entry_bno = find_fat_blockno(fs->below, last_entry);
            /**
             * Read out the last fat entry's fatblock, we need to modify it, set up
             * its next pointer
             */
            if ((*fs->below->read)(fs->below, last_entry_bno, (block_t *) &lastfatblock) < 0) {
                panic("fatdisk_write: addblocks\n");
                return -1;
            }
            lastfatblock.fatblock.entries[last_entry % FAT_PER_BLOCK].next = free_bno;
            //write the fatblock into disk
            if ((*fs->below->write)(fs->below, last_entry_bno, (block_t *) &lastfatblock) < 0) {
                panic("fatdisk_write: update fat table\n");
                return -1;
            }

        }

        //update inode block if needed
        if (offset >= snapshot.inode->nblocks) {
            snapshot.inode->nblocks = offset + 1;
            dirty_inode = 1;
        }

        if(dirty_inode) {
            if ((*fs->below->write)(fs->below, snapshot.inode_blockno, (block_t *) &snapshot.inodeblock) < 0) {
                panic("treedisk_write: inode block");
                return -1;
            }
        }
        last_entry = free_bno;
    }
    /**
     * finishing allocating, write the block into offset position
     */
    fatentry_no entry_for_data = fatdisk_traverse(this_bs, offset);
    block_no data_block = fatentry_to_datablock(entry_for_data, fs->below);

    int r =  (*fs->below->write)(fs->below, data_block, block);


    return r;
}

/* Read a block at the given block number 'offset' and return in *block.
 */
static int fatdisk_read(block_store_t *this_bs, block_no offset, block_t *block){
    /**
     * Get meta info of this inode
     */
    struct fatdisk_state *ft = this_bs->state;

    struct fatdisk_snapshot snapshot;

    if(fatdisk_get_snapshot(&snapshot, ft->below, ft->inode_no) < 0) return -1;


    if(offset >= snapshot.inode->nblocks) {
        fprintf(stderr, "!!TDERR: fatdisk_read offset too large\n");
        return -1;
    }

    block_no start = snapshot.inode->head;
    for(;;) {
        if(offset == 0) {
            /**
             * Now we are at the wanted block, just read it
             */
            block_no readfrom = fatentry_to_datablock(start,ft->below);
            int result = (*ft->below->read)(ft->below, readfrom, block);
            return result;

        } else {
            /**
             * Not yet, need to traverse the inode's linklist
             */
            union fatdisk_block fatblock;
            block_no fat_bno = find_fat_blockno(ft->below, start);

            if((*ft->below->read)(ft->below, fat_bno, (block_t *) &fatblock) < 0) return -1;

            fatentry_no index = start % FAT_PER_BLOCK;
            fatentry_no next = fatblock.fatblock.entries[index].next;
            start = next;
        }
        offset--;
    }

}


/* Get size.
 */
static int fatdisk_nblocks(block_store_t *this_bs){
    struct fatdisk_state *fs = this_bs->state;

    /* Get info from underlying file system.
     */
    struct fatdisk_snapshot snapshot;
    if (fatdisk_get_snapshot(&snapshot, fs->below, fs->inode_no) < 0) {
        return -1;
    }

    return snapshot.inode->nblocks;
}

/* Set the size of the file 'this_bs' to 'nblocks'.
 */
static int fatdisk_setsize(block_store_t *this_bs, block_no nblocks){
    struct fatdisk_state *fs = this_bs->state;

    struct fatdisk_snapshot snapshot;
    fatdisk_get_snapshot(&snapshot, fs->below, fs->inode_no);
    if (nblocks == snapshot.inode->nblocks) {
        return nblocks;
    }
    if (nblocks > 0) {
        fprintf(stderr, "!!TDERR: nblocks > 0 not supported\n");
        return -1;
    }

    fatdisk_free_file(&snapshot, fs->below);
    return 0;
}

static void fatdisk_destroy(block_store_t *this_bs){
    free(this_bs->state);
    free(this_bs);
}


/* Create or open a new virtual block store at the given inode number.
 */
block_store_t *fatdisk_init(block_store_t *below, unsigned int inode_no){
    /* Get info from underlying file system.
     */
    struct fatdisk_snapshot snapshot;
    if (fatdisk_get_snapshot(&snapshot, below, inode_no) < 0) {
        return 0;
    }

    /* Create the block store state structure.
     */
    struct fatdisk_state *fs = new_alloc(struct fatdisk_state);
    fs->below = below;
    fs->inode_no = inode_no;

    /* Return a block interface to this inode.
     */
    block_store_t *this_bs = new_alloc(block_store_t);
    this_bs->state = fs;
    this_bs->nblocks = fatdisk_nblocks;
    this_bs->setsize = fatdisk_setsize;
    this_bs->read = fatdisk_read;
    this_bs->write = fatdisk_write;
    this_bs->destroy = fatdisk_destroy;
    return this_bs;
}

//#endif

