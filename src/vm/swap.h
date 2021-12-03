#ifndef SWAP_H
#define SWAP_H
#include <bitmap.h>
#include "devices/block.h"
#include "threads/vaddr.h"
#include "threads/synch.h"

static struct bitmap *swap_table;
static struct block *swap_block_table;
static struct lock swap_lock;

static const size_t NUM_SECTOR_PER_PAGE = PGSIZE / BLOCK_SECTOR_SIZE;

void swap_init();
void swap_in(size_t swap_index, void* addr);
size_t swap_out(void* addr);
#endif