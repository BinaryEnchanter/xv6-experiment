// Kernel malloc implementation
// func malloc() && free() are implemented here.
// based on slab,but this is a simple version. That is this slab use kalloc() as API to mem
// instead of implemented on buddy system
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"

//heap has 16MB in the free mem of kernel



#define MIN_BLOCK_SIZE (512)
struct free_block {
    struct free_block *next;
    uint size;
    uint magic;// identify the alloced block
};

static struct free_block *free_list = 0;  

static struct spinlock freeLISTlock;

// This is for initializing area of first fit
void list_init() {
    printf("[INFO]:Initiating free list heap memory\n");
    initlock(&freeLISTlock,"list_mem");
    free_list = (struct free_block *)HEAP_MIDDLE;
    free_list->size = PHYSTOP-HEAP_MIDDLE;
    free_list->next = 0;
    printf("[INFO]:list_init success!\n");
}

void *list_alloc(uint size) {
    acquire(&freeLISTlock);
    struct free_block **prev = &free_list;
    struct free_block *cur = free_list;
    //align to size of the struct, in case that the free block can't even hold one struct head
    size = (size + sizeof(struct free_block) - 1) & ~(sizeof(struct free_block) - 1); 

    uint actual_size=size+sizeof(struct free_block);
    uint min_free_block=sizeof(struct free_block)+(int)MIN_BLOCK_SIZE;
    while (cur) {
        if (cur->size >= actual_size) {
            // if current block size is bigger than minimal block size(struct+512B),then split
            if (cur->size >= actual_size+min_free_block) {
                // get needed size of block
                struct free_block *new_block = (struct free_block *)((char *)cur + actual_size);
                new_block->size = cur->size - actual_size;
                new_block->next = cur->next;
                new_block->magic = 0xDCBA;
                *prev = new_block;

                cur->size=actual_size;
                //cur->next=new_block;
            } else {
                // no need of split
                *prev = cur->next;
            }
            
            release(&freeLISTlock);
            printf("Free list alloc successfully");
            return (void *)((char *)cur + sizeof(struct free_block));
        }
        // go to next free block
        prev = &cur->next;
        cur = cur->next;
    }
    
    release(&freeLISTlock);
    printf("Free list alloc failed");
    return 0;  
}

void list_free(void *ptr) {
    acquire(&freeLISTlock);
    struct free_block *block = (struct free_block *)((char *)ptr - sizeof(struct free_block));
    if (block->magic!=0xDCBA){
        release(&freeLISTlock);
        printf("Wrong free target, target not in freelist_heap.\n The magic num is %x,not 0xDCBA\n",block->magic);
        return;
    }
    //uint size = block->size;

    struct free_block **prev = &free_list;
    struct free_block *cur = free_list;

    // find the place of the block,to find the prev
    while (cur && cur < block) {
        prev = &cur->next;
        cur = cur->next;
    }

    // insert into free list
    block->next = cur;
    //int prev_size=(*prev)->size;
    (*prev)= block;
    printf("recycle block successfully,new free block size is %d\n",block->size);
    // try to merge blocks
    if (cur && (char *)block + block->size == (char *)cur) {
        block->size += cur->size;
        block->next = cur->next;
        printf("Merge to next block,new size is %d\n",block->size);
    }
    uint *prev_size=(uint*)((char*)prev+sizeof(struct free_block *));
    //printf("prev:%p,prev size:%x,block:%p,*prev+size=%p\n",prev,*prev_size,block,(char *)(prev) + *prev_size);
    if (*prev && (char *)prev + *prev_size == (char *)block) {
        *prev_size += block->size;
        (*prev) = block->next;
        printf("Merge to last block,new size is %d\n",block->size);
    }
    release(&freeLISTlock);
}
void freelist_state() {
    acquire(&freeLISTlock);
    struct free_block *current = free_list;
    int count = 0;
    uint total_free = 0;
  
    printf("========== Free List Usage ==========\n");
    printf("| %s | %s | %s |\n", "No.", "Block Address", "Size (bytes)");
    printf("----------------------------------------------\n");
  
    while(current) {
        printf("| %d | %p | %u |\n", count, current, current->size);
        total_free += current->size;
        count++;
        current = current->next;
    }
    printf("----------------------------------------------\n");
    printf("Total free memory: %d bytes in %d block(s).\n", total_free, count);
    printf("==============================================\n");
    release(&freeLISTlock);
  }
  