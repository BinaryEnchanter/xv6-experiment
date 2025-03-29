#ifndef RAND_H
#define RAND_H
#include "types.h"
#include "param.h"
#include "riscv.h"
#include "proc.h"
#include "defs.h"

// in case of competition, each CPU maintain its own seed
#define RAND_A 1664525
#define RAND_C 1013904223
#define RAND_M (1U << 31)

static uint32 seeds[NCPU];  


void rand_init() {
    for (int i = 0; i < NCPU; i++) {
        seeds[i] = i + 12345;  
    }
}


void srand(uint32 s) {
    int id = cpuid();  
    seeds[id] = s;
}

// LCG rand
uint32 rand() {
    int id = cpuid();
    seeds[id] = seeds[id] * RAND_A + RAND_C;
    return (seeds[id] & (RAND_M - 1));  //31bit rand
}
#endif