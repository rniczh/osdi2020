#pragma once

#include "buddy.h"

struct SlabEntry {
  struct SlabEntry *next;
};

struct Slab {
  struct Slab *next;
  struct SlabEntry *free_list;

  unsigned long start;
  int size;
  int free_length;
  int length;
  int fixed;
};

extern unsigned long global_mem_start;
extern struct Slab* global_slab_meta;
extern struct Slab* global_slab_list;

void *kalloc(unsigned long size);
void kfree(void *ptr);
void kalloc_init(struct buddy *bd);
struct Slab *slab_meta_alloc(unsigned long slabStart);
void init_slab(struct Slab *self, unsigned long start, int size);
