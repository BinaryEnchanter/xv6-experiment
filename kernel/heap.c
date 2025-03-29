//#include "types.h"
//#include "defs.h"
//#include "memlayout.h"
//#include "spinlock.h"
#include "slab.h"
#include "freelist.h"
#include "rand.h"

#define MAX_SLAB_SIZE (256)
#define NUM_CHILD    1        
#define NUM_ALLOCS   100     
#define MIN_ALLOC    16       
#define MAX_ALLOC    4096//1048576  

//the global slab caches array

void slab_state();
void slab_test();
void freelist_test();
void random_test();
void rand_init();
uint32 rand();
void stress_test(int);
void multi_core_test();
void
heap_init()
{
    rand_init();
    printf("Calling slab_init\n");
    slab_init(slab_caches);
    printf("Calling list_init\n");
    list_init();
    printf("heap_init success!\n");
    printf("=======================================\n");
}

void* 
malloc(uint size) 
{
    printf(">>-------func_malloc:Mallocing mem...\n");
    if (size == 0||size>HEAP_SIZE/2) return 0;
  
    // Acquiring mem<=256B
    if (size <= MAX_SLAB_SIZE) {
      for (int i = 0; i < SLAB_SIZES; i++) {
        if (slab_sizes[i] >= size) {
          void *obj = slab_alloc(slab_caches[i]);
          if (obj) {
            printf("Slab alloc %d\n",slab_sizes[i]);
            return obj;
          }
          //if fail,then go to free list
        }
      }
    }
  
    // slab fail or mem>256B
    void *obj=list_alloc(size);
    if (obj){
      printf("Free list alloced\n ");
      return obj;
    }
    
    printf("-------func_malloc:fail to malloc\n");
    return 0;
  }

  void 
  free(void *ptr) 
  {
    printf(">>-------func_free:freeing mem\n");
    if (!ptr||(uint64)ptr<(uint64)HEAP_START||(uint64)ptr>(uint64)PHYSTOP) {
        printf("-------func_free:ptr is invalid,ptr:%lx\n",(uint64)ptr);
        return;
    }
    //check if is of slab
    if ((uint64)HEAP_START<=(uint64)ptr&&(uint64)ptr<=(uint64)HEAP_MIDDLE)
    {
      printf("Entering slab_free\n");
      //struct slab *s = (struct slab *)((uint64)ptr & ~(PGSIZE - 1));
        
          slab_free(slab_caches, ptr);
          //printf("free success");
          return;
    }
    printf("Entering list_free\n");
    list_free(ptr);
  }

 uint64 sys_malloctest(void) {
    // slab_state();
    // freelist_state();
    //void* ptr=malloc(1024);
    
    // slab_test();
    // freelist_test();

    // printf("Malloc test completed.\n");
    
    //stress_test(1);
    //random_test();
    //multi_core_test();
    int mode;
    argint(0, &mode);
    switch ((char)mode)
    {
    case 'a':
      slab_test();
      slab_state();
      break;
    case 'b':
      freelist_test();
      freelist_state();
      break;
    
    case 'c':
      int testnum=100;
      void **ptrs = malloc(testnum * sizeof(void*));
      for(int i = 0; i < testnum; i++){
        int size = (rand() % (MAX_ALLOC - MIN_ALLOC + 1)) + MIN_ALLOC;
        printf("[No.%d],malloc:%d\n",i,size);
        ptrs[i]=malloc(size);
        if (!ptrs[i]) printf("Error: malloc failed at %d\n", i);
      }
      for (int i = 0; i < testnum; i++) {
        printf("[No.%d]\n",i);
        free(ptrs[i]);
      }
      free(ptrs);
      break;
    case 'e':
    printf("=========Boundary and Validity Test========\n");
      void *ptr = malloc(0); 
      printf("Allocating 0 bytes correctly returned %p\n",ptr);
      void *ptr0 = malloc(HEAP_SIZE); 
      printf("Allocating large size correctly returned %p\n",ptr0);
      printf("--->free 0x7B\n");
      free((void*)123);
      printf("--->free 0x0000000087800011,this is in slab range\n");
      free((void*)0x0000000087800011);
      printf("--->free 0x0000000087c00011,this is in freelist range\n");
      free((void*)0x0000000087c00011);
      printf("=====================================\n");
      break;
    default:
      slab_state();
      freelist_state();
      break;
    }
    
      
    return 0;
  }

  //slab test
  void slab_test()
  {
    printf("=============Slab Test==============\n ");
    printf("here");
    //int testnum=16384;
    int testnum=100;
    //void *ptrs[testnum];
    void **ptrs = malloc(testnum * sizeof(void*));
    for (int i = 0; i < testnum; i++) {
      printf("[No.%d]\n",i);
      ptrs[i] = malloc(256);
      if (!ptrs[i]) printf("Error: malloc failed at %d\n", i);
    }
  
    for (int i = 0; i < testnum; i++) {
      printf("[No.%d]\n",i);
      free(ptrs[i]);
    }
    free(ptrs);
  }
  //freelist test
  void freelist_test()
  {
    printf("=============Free List Test==============\n ");
    int testnum=100;
    void **ptrs = malloc(testnum * sizeof(void*));
    for (int i = 0; i < testnum; i++) {
      printf("[No.%d]\n",i);
      ptrs[i] = malloc(1000);
      if (!ptrs[i]) printf("Error: malloc failed at %d\n", i);
    }
  
    for (int i = 0; i < testnum; i++) {
      printf("[No.%d]\n",i);
      free(ptrs[i]);
    }
    free(ptrs);
  }
  

  
  
  void multi_core_test() {
    int i;
    printf("=====  Multi-Core Test =====\n");
    printf("\n Initial state:\n");
    slab_state();
    freelist_state();
    // malloc
    for(i = 0; i < NUM_CHILD; i++){
      int pid = fork();
      if(pid < 0){
        printf("multi_core_test: fork failed!\n");
        return;
      }
      if(pid == 0){
        
        for(i = 0; i < NUM_ALLOCS; i++){
          
          int size = (rand() % (MAX_ALLOC - MIN_ALLOC + 1)) + MIN_ALLOC;
          malloc(size);
      }
      exit(0);
    }
    for(i = 0; i < NUM_ALLOCS; i++){
          
      int size = (rand() % (MAX_ALLOC - MIN_ALLOC + 1)) + MIN_ALLOC;
      malloc(size);
  }
    
    for(i = 0; i < NUM_CHILD; i++){
      pid=wait(0);
      printf("ppid is %d\n",pid);
    }
    slab_state();
    freelist_state();
    printf("===== Multi-Core  Test Complete =====\n");
  }
  }  