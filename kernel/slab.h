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

#define SLAB_BASE HEAP_START
#define SLAB_STOP HEAP_MIDDLE
#define SLAB_SIZES 5

// the slab_cache array
//static struct slab_cache* caches[SLAB_SIZES];

// kind of slab_cache
static int slab_sizes[SLAB_SIZES] = {16, 32, 64, 128, 256};
// top of slab heap MEM
static char* freeMEMtop= (char*)SLAB_BASE;
// Protector of above
static struct spinlock freeMEMlock;
// Global free list of slab
static struct slab *free_slab_list=0;
// magic num for slab
//static int magic[SLAB_SIZES]={0xABCD0010,0xABCD0020,0xABCD0040,0xABCD0080,0xABCD0100};

// slab caches
static struct slab_cache* slab_caches[SLAB_SIZES]; 
//help func
int log2_16(unsigned int x) {
  if (x == 0) return -1; 
  int res = 0;
  if (x >= 8) { res += 3; x >>= 3; } // x >= 8
  if (x >= 4) { res += 2; x >>= 2; } // x >= 4
  if (x >= 2) { res += 1; x >>= 1; } // x >= 2
  return res;
}

// Slab of PGSIZE
// Struct slab exists in the front of a slab
// Size of 16B
struct slab {
  struct slab *next;   
  void *free_list;     // Refer to top of free list
  int inuse;           // Number of alloced objs
  int magic;           //1 for referring obj-size,2 for checking validity of obj*
};

// The head of slab link list
// Size of 24B
struct slab_cache {
  struct spinlock lock; 
  struct slab *slab_list; // All slabs which has the same obj size
  struct slab *quick_slab;  //the end slab,used for quick locate
  int object_size;        
  int objects_per_slab;   
};
//   DECLERATION
static struct slab* slab_create_new(struct slab_cache *cache);

// // Create slab_cache, 
// // object_size define the size of objects in the slab_cache
// struct slab_cache* slab_create_cache(int object_size) {
//   struct slab_cache *cache;
//   cache = kalloc(); 
//   if(cache == 0)
//     return 0;
//   initlock(&cache->lock, "slab_cache");
//   
//   cache->object_size = (object_size < sizeof(void*)) ? sizeof(void*) : object_size;
//   
//   cache->objects_per_slab = (PGSIZE - sizeof(struct slab)) / cache->object_size;
//   cache->slab_list = 0;
//   return cache;
// }

// Input which slab_caches you want to init
// all slab caches store in one page from kalloc()
void slab_init(struct slab_cache* caches[]) {
  printf("[INFO]:Initiating slab Memory\n");

  initlock(&freeMEMlock, "slab_mem");
  char*page=(char*)kalloc();

  for (int i = 0; i < SLAB_SIZES; i++) {
      struct slab_cache *cache = (struct slab_cache*) page;
      page += sizeof(struct slab_cache);
      
      initlock(&cache->lock, "slab_cache");
      cache->object_size = slab_sizes[i];
      cache->objects_per_slab = (PGSIZE - sizeof(struct slab)) / cache->object_size;
      cache->slab_list = 0; //initial there were none list,create only one initially
      cache->slab_list=slab_create_new(cache);
      cache->quick_slab=cache->slab_list;
      caches[i] = cache;
      printf("Slab cache with size of %d has alloced\n",slab_sizes[i]);
  }
  printf("[INFO]:slab_init success!\n");
}
// create new slab for a certain slab_cache
static struct slab* slab_create_new(struct slab_cache *cache) {
  struct slab *s;
  // procedure of allocing mem for a new slab
  acquire(&freeMEMlock);
  // Priority is to use free_slab_list
  if (free_slab_list) {
    s = free_slab_list;
    free_slab_list = free_slab_list->next;
    release(&freeMEMlock);
    printf("Slab mem alloced from free_slab_list successfully\n");
  }
  else if ((uint64)freeMEMtop + PGSIZE > (uint64)SLAB_STOP) {
    release(&freeMEMlock);
    printf("In 'slab.c' func slab_create_new(), no more free mem for slab\n");
    return 0; 
  }
  else {
    s = (struct slab*) freeMEMtop;// this is the new slab
    freeMEMtop += PGSIZE;
    release(&freeMEMlock);
    printf("Slab mem alloced from free_slab_mem successfully\n");
  }


  s->inuse = 0;
  // slab head store in the head the mem
  char *obj_start = (char*)s + sizeof(struct slab);
  s->free_list = (void*) obj_start;
  // initiating of slab 
  for(int i = 0; i < cache->objects_per_slab - 1; i++) {
    char *obj = obj_start + i * cache->object_size;
    char *next = obj_start + (i+1) * cache->object_size;
    *((void**)obj) = (void*) next;
  }
  // last obj's nest is NULL
  char *last_obj = obj_start + (cache->objects_per_slab - 1) * cache->object_size;
  *((void**)last_obj) = 0;
  s->next = 0;
  
  s->magic=0xABCD0000+cache->object_size;
  return s;
}

// alloc mem for obj from slab
void* slab_alloc(struct slab_cache *cache) {
  acquire(&cache->lock);
  struct slab *s = cache->slab_list;
  struct slab *quick_s=cache->quick_slab;
  // if the newest slab is full
  if (s->free_list==0){
    // check first 10 slab after the quick_s && itself
    int i=0;
    while (i<10&&quick_s&&quick_s->free_list==0)
    {
      i++;
      quick_s=quick_s->next;
      printf("Current slab full\n");
    }
    // if above fail,new a slab
    if (i==10||quick_s==0){
      s = slab_create_new(cache);
        if(s == 0) {
          release(&cache->lock);
          printf("Creating new slab failed\n");
          return 0;
        }
    
        s->next = cache->slab_list;
        cache->slab_list = s;
        // if quick_s reach the end, reset to the second slab
        quick_s=quick_s?quick_s:cache->slab_list->next;
    }else{
      s=quick_s;
      cache->quick_slab=quick_s;
    }
  }
  
  
  // get mem for obj
  void *obj = s->free_list;

  s->free_list = *((void**) obj);
  s->inuse++;
  release(&cache->lock);
  return obj;
}

// Release mem of obj
void slab_free(struct slab_cache *caches[], void *obj) {
  // we can get slab addr by set "void *obj"'s low bits as 0
  struct slab *s = (struct slab *) ((uint64)obj & ~(PGSIZE - 1));
  unsigned magic=s->magic&0xffffffff;
  if (magic>>16!=(unsigned)0xABCD)
  {
    printf("Wrong free target, target not in slab_heap.\n The magic num is %x,not ABCD\n",magic>>16);
    return;
  } 
  int index=log2_16((magic&0xffff)/16);
  struct slab_cache* cache=caches[index];
  
  acquire(&cache->lock);
  if (((char *)obj - (char*)s-sizeof(struct slab)) % cache->object_size != 0) {
    printf("slab_free: pointer %p is misaligned relative to object size %d\n", obj, cache->object_size);
    release(&cache->lock);
    return;
  }
  // update slab head
  *((void**)obj) = s->free_list;
  s->free_list = obj;
  s->inuse--;
  printf("Free obj successfully from slab_cache with size of %d,in slab cache of %d\n",cache->object_size,slab_sizes[index]);
  // if slab is empty,all unused,recycle the slab
  if(s->inuse == 0) {
    printf("Now slab is empty,recycling...\n");
    if (cache->slab_list->next==0){
      printf("This is the last slab of the cache, recycle terminated\n");
    }else
    {
      // remove the slab from slab_cache
      struct slab **pp = &cache->slab_list;
      while(*pp && *pp != s) {
        pp = &((*pp)->next);
      }
      if(*pp == s) {
        *pp = s->next;
      }
      
      // move the empty slab to free_slab_list
      acquire(&freeMEMlock);
      s->next = free_slab_list;
      free_slab_list = s;  
      release(&freeMEMlock);
    }
    
    
    release(&cache->lock);
    printf("recycle success\n");
    return;
  }
  release(&cache->lock);
}
  //Show the state of slab cache
  void slab_state() {
    acquire(&freeMEMlock);
    printf("\n================ Slab State =================\n");
    // for each cache
    for (int i = 0; i < SLAB_SIZES; i++) {
      struct slab_cache *cache = slab_caches[i];
      if (cache == 0) {
        printf("Slab Cache[%d]: Not initialized.\n", i);
        continue;
      }
      printf("Slab Cache[%d]: Object Size: %d, Objects per Slab: %d\n",
             i, cache->object_size, cache->objects_per_slab);
      
      // for each slab
      struct slab *s = cache->slab_list;
      int slab_num = 0;
      while (s) {
        // for each free node
        int free_count = 0;
        void *p = s->free_list;
        while (p) {
          free_count++;
          p = *((void **)p);
        }
        printf("  Slab %d @ %p: In use: %d, Free: %d, Magic: 0x%x\n",
               slab_num, s, s->inuse, free_count, s->magic);
        s = s->next;
        slab_num++;
      }
      printf("------------------------------------------------\n");
    }
    printf("============================================\n");
    release(&freeMEMlock);
  }