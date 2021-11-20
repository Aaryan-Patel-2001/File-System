#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "cache.h"

static cache_entry_t *cache = NULL;
static int cache_size = 0;
static int clock = 0;
static int num_queries = 0;
static int num_hits = 0;

int cache_create(int num_entries) {
  // Both if and else if check for invalid parameters. 
  if (num_entries<2 || num_entries>4096){
    return -1;
  }
  else if (cache!=NULL){
    return -1;
  }
  else{
    cache = calloc(num_entries, sizeof(cache_entry_t));
    cache_size = num_entries;
    return 1;
  }
}

int cache_destroy(void) {
  // if checks for invalid parameters. else is default case. 
  if (cache==NULL){
    return -1;
  }
  else{
    free(cache);
    cache = NULL;
    cache_size = 0;
    return 1;
  }
}

int cache_lookup(int disk_num, int block_num, uint8_t *buf) {
  // if and else if's check for invalid parameters.
  /* This function checks if a disk_num and block_num exists in cache or not. If it does then
     it increments num_hits and updates the element's access_time.
     Returns 1 on SUCCESS and -1 on FAILURE.  
   */
  if (cache==NULL){
    return -1;
  }
  else if(cache_size==0){
    return -1;
  }
  else if(buf==NULL){
    return -1;
  }
  else{
    num_queries +=1;
    int i;
    for(i=0; i<cache_size; i++){
      if (cache[i].block_num==block_num&&cache[i].disk_num==disk_num&&cache[i].valid==true){
        memcpy(buf, cache[i].block, JBOD_BLOCK_SIZE);
        num_hits+=1;
        clock+=1;
        cache[i].access_time=clock;
        return 1;
      }
    }
    return -1;
  }
}

void cache_update(int disk_num, int block_num, const uint8_t *buf){
  // unconditionally checks if a disk_num and block_num exists in cache and if it does then updates it value. 
  int i;
  for(i=0; i<cache_size; i++){
    if (cache[i].block_num==block_num&&cache[i].disk_num==disk_num && cache[i].valid==true){
      memcpy(cache[i].block, buf, JBOD_BLOCK_SIZE);
      clock+=1;
      cache[i].access_time=clock;
    }
  }
}

int cache_insert(int disk_num, int block_num, const uint8_t *buf) {
  // if and else if's check for invalid parameters
  /*
this function tries to insert into cache an element with disks_num, block_num, and buf
its follows LRU policy. 
   */
  if (cache==NULL){
    return -1;
  }
  else if(buf==NULL){
    return -1;
  }
  else if (disk_num<0 || disk_num>15){
    return -1;
  }
  else if (block_num<0 || block_num>255){
    return -1;
  }
  else{
    // search if entry ALREADY EXISTS, if it does then UPDATE it's value
    int p;
    for(p=0; p<cache_size; p++){
      if (cache[p].block_num==block_num && cache[p].disk_num==disk_num&&cache[p].valid==true){
        // inserting with same value should fail
        if (memcmp(cache[p].block, buf, JBOD_BLOCK_SIZE)==0){
	  return -1;
	}
        memcpy(cache[p].block, buf, JBOD_BLOCK_SIZE);
        clock+=1;
        cache[p].access_time=clock;
        return 1;
      }
    }
    // search if there is an empty space in cache. If there is, then it's cache[i].valid would be false 
    int i;
    for(i=0; i<cache_size; i++){
      if (cache[i].valid==false){
        cache[i].valid=true;
        cache[i].disk_num=disk_num;
        cache[i].block_num=block_num;
        clock+=1;
        cache[i].access_time=clock;
        memcpy(cache[i].block, buf, JBOD_BLOCK_SIZE);
        return 1;
      }
    }
    // if there is no empty space, try to find item with lowest access_time and replace i with current item. 
    int j;
    int lowest = 0;
    for(j=0; j<cache_size; j++){
      if (cache[j].access_time<cache[lowest].access_time){
        lowest=j;
      }
    }
    cache[lowest].disk_num=disk_num;
    cache[lowest].block_num=block_num;
    clock+=1;
    cache[lowest].access_time=clock;
    memcpy(cache[lowest].block, buf, JBOD_BLOCK_SIZE);
    return 1;
  }
}

bool cache_enabled(void) {
  // checks if cache is enabled. Helpful for mdadm implementation
  if(cache_size==0){
    return false;
  }
  else{
    return true;
  }
}

void cache_print_hit_rate(void) {
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float) num_hits / num_queries);
}
