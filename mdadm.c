#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mdadm.h"
#include "jbod.h"

void *memcpy(void *dest, const void * src, size_t n);

#define MAX_LEN  1024 // maximum number of bytes that can be read or written. Max value of len parameter in mdadm_read and mdadm_write

static int MachineState = 0; //keeps track within this implementation on whether system is mount or unmount. 0 is unmount, 1 represents mount. 

uint32_t encode_operation(jbod_cmd_t cmd, int disk_num, int block_num){ 
  uint32_t d = disk_num & 0x0000000f;
  uint32_t b = block_num & 0x000000ff;
  uint32_t op = (cmd << 26) | (d << 22) | (b);
  return op;
}

int mdadm_mount(void){
  if (MachineState == 0){
    uint32_t op = encode_operation(JBOD_MOUNT, 0, 0);
    jbod_operation(op, NULL);
    MachineState = 1;
    return 1;
  }
  else{
    return -1;
  }
}


int mdadm_unmount(void){
  if (MachineState == 1){
    uint32_t op = encode_operation(JBOD_UNMOUNT, 0, 0);
    jbod_operation(op, NULL);
    MachineState = 0;
    return 1;
  }
  else{
    return -1;
  }
}

void translate_address(uint32_t linear__addr, int *disk_num, int *block_num, int *offset){
  *disk_num = linear__addr / JBOD_DISK_SIZE;
  *block_num = (linear__addr%JBOD_DISK_SIZE)/JBOD_BLOCK_SIZE;
  *offset = (linear__addr%JBOD_DISK_SIZE)%JBOD_BLOCK_SIZE;
}

void seek(int disk_num, int block_num){
 uint32_t op1 =  encode_operation(JBOD_SEEK_TO_DISK, disk_num, 0);
 jbod_operation(op1, NULL);
 uint32_t op2 =  encode_operation(JBOD_SEEK_TO_BLOCK, 0, block_num);
 jbod_operation(op2, NULL);
}

int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf){
  if (len == 0){
    return 0;
  }
  else if(MachineState==0){
    return -1;
  }
  else if((addr+len)>(JBOD_NUM_DISKS*JBOD_DISK_SIZE)){
    return -1;
  }
  else if(len>MAX_LEN){
    return -1;
  }
  else if((buf==NULL) && (len!=0)){
    return -1;
  }
  else{
    uint32_t curr_addr;
    int disk_num, block_num, offset;
    int data = 0;
    uint32_t op;
    uint8_t  buf1[JBOD_BLOCK_SIZE];
    int x;
    for(curr_addr=addr; curr_addr<addr+len; curr_addr = addr + data)
      {
	bzero(buf1, JBOD_BLOCK_SIZE);
        translate_address(curr_addr, &disk_num, &block_num, &offset);
	if(cache_enabled()==true){
	  x = cache_lookup(disk_num, block_num, buf1);
	}
        if(cache_enabled()==false || x==-1){
          seek(disk_num, block_num);
          op = encode_operation(JBOD_READ_BLOCK, 0,0);
          jbod_operation(op, buf1);
	  if (cache_enabled()==true && x==-1){
	    cache_insert(disk_num, block_num, buf1);
	  }
	}
	
        if (offset>0 && offset<=JBOD_BLOCK_SIZE-1){
          memcpy(buf+data, buf1+offset, JBOD_BLOCK_SIZE-offset);
          data += JBOD_BLOCK_SIZE-offset;
	}
        else if (len - data<JBOD_BLOCK_SIZE){
          memcpy(buf+data, buf1, len-data);
          data += len - data;
        }

        else{
          memcpy(buf+ data, buf1, JBOD_BLOCK_SIZE);
          data += JBOD_BLOCK_SIZE;
        }
      }
     return len;
  }
}

int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf) {
  if (len == 0){
    return 0;
  }
  else if(MachineState==0){
    return -1;
  }
  else if((addr+len)>(JBOD_NUM_DISKS*JBOD_DISK_SIZE)){
    return -1;
  }
  else if (len>MAX_LEN){
    return -1;
  }
  else if((buf==NULL) && (len!=0)){
    return -1;
  }
  else {
    uint32_t curr_addr;
    int disk_num, block_num, offset;
    int  data = 0;
    uint8_t buf1[JBOD_BLOCK_SIZE];
    uint32_t op;
    for(curr_addr=addr; curr_addr<addr+len; curr_addr = addr + data){
      translate_address(curr_addr, &disk_num, &block_num, &offset);
      bzero((void *)buf1, JBOD_BLOCK_SIZE);
      mdadm_read(curr_addr-offset, JBOD_BLOCK_SIZE,buf1);
      seek(disk_num, block_num);
      op = encode_operation(JBOD_WRITE_BLOCK, 0,0);
      if (offset>0&&offset<=JBOD_BLOCK_SIZE-1){
	if(len<JBOD_BLOCK_SIZE-offset){
	  memcpy(buf1+offset, buf+data, len);
	  cache_update(disk_num, block_num, buf1);
	  jbod_operation(op,buf1);
	  data = len;
	  return len;
	}
	else{
	  memcpy(buf1+offset, buf+data, JBOD_BLOCK_SIZE-offset);
	  cache_update(disk_num, block_num, buf1);
	  jbod_operation(op, buf1);
	  data+=JBOD_BLOCK_SIZE-offset;
	}
      }
      else if (len - data<JBOD_BLOCK_SIZE){
	memcpy(buf1+offset, buf+data, len-data);
	cache_update(disk_num, block_num, buf1);
        jbod_operation(op, buf1);
	data = len;
	return len;
	}
      else{
	memcpy(buf1+offset, buf+data, JBOD_BLOCK_SIZE);
	cache_update(disk_num, block_num, buf1);
        jbod_operation(op, buf1);
	data+=JBOD_BLOCK_SIZE;
	}	
      }
    return len;
    }
}
