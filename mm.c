/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

size_t free_space;

int mm_init(void)
{
    free_space = 0; 
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *add_block_to_heap(size_t size_after_alignment){
    void *tmp = mem_sbrk(size_after_alignment);
        if(tmp == (void *)-1) { //error in mem_sbrl
            return NULL;
        }else{
            *(size_t *) tmp = size_after_alignment | 1; // head boundary_tag (length + used)
            *(size_t *) (tmp + size_after_alignment - SIZE_T_SIZE) = size_after_alignment | 1; //footer boundary_tag (length + used)
            return (void*) (tmp  + SIZE_T_SIZE);
        }
}

char *first_fit(size_t size_needed){
    char *start = mem_heap_lo();
    char *end = mem_heap_hi();
    while(start <= end &&                   // still inside the heap
          (((*(size_t*) start) & 1) == 1 ||     // free or not
          (*(size_t*) start) < size_needed    // enough space inside
          ))
    {
        start += (*(size_t*) start) & -2;
    }
    if(start > end){
        return NULL;
    }else{
        return start;
    }
}

void *add_in_free_block(char *fit_pointer, size_t size_after_alignment){
            size_t old_size = *(size_t*)fit_pointer;
            *(size_t*)fit_pointer = (size_after_alignment | 1);
            *(size_t*)(fit_pointer + size_after_alignment - SIZE_T_SIZE) = size_after_alignment | 1;
            size_t rest = old_size - size_after_alignment;
            if(rest > 0){
                *(size_t*)(fit_pointer + size_after_alignment) = rest;
                *(size_t*)(fit_pointer + old_size - SIZE_T_SIZE) = rest;
            }
            return (void *) (fit_pointer + SIZE_T_SIZE);
}

void *mm_malloc(size_t size)
{
    size_t size_after_alignment = ALIGN(size + 2 * SIZE_T_SIZE); // 2 blocks reserved for tagging
    if(free_space < size_after_alignment){ // for sure not enough space inside the current heap
        return add_block_to_heap(size_after_alignment); // add a new block to the heap
    }else{
        char *first_fit_pointer = first_fit(size_after_alignment); 
        if(first_fit_pointer == NULL){ // couldn't find a good block
            return add_block_to_heap(size_after_alignment); // add a n block to the heap
        }else{ // there is a free block in the current heap - add the payload there
            free_space -= size_after_alignment;
            return add_in_free_block(first_fit_pointer, size_after_alignment); 
        }
    }  
}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptrrr)
{
    char *ptr = (char *) (ptrrr - SIZE_T_SIZE); // go to metadata
    (*(size_t *)ptr) &= -2; // bc of the flag
    (*(size_t *)(ptr + *(size_t *)(ptr) - SIZE_T_SIZE)) &= -2;//bc of the flag
    free_space += *(size_t *)ptr;
    char *block_back = (ptr - SIZE_T_SIZE);
    char *block_front = (ptr + (*(size_t *) ptr));
    // //MERGE_BACK
    if(ptr != mem_heap_lo()){ //i.e. MAKE SURE TO HAVE A BACK - IF ptr is the first block in the heap -> no back block
         if(((*(size_t *) block_back) & 1) == 0){
             block_back -= (*(size_t *) block_back - SIZE_T_SIZE);
             *(size_t *)block_back = *(size_t *) (block_front - SIZE_T_SIZE) = *(size_t *) block_back + *(size_t *) ptr; 
             ptr = block_back;
         }
    }
    //MERGE _FRONT
    if(block_front < (char *)mem_heap_hi()){
         if(((*(size_t *) block_front) & 1) == 0){
            *(size_t *) ptr = *(size_t*) (block_front + *(size_t*)block_front - SIZE_T_SIZE) = *(size_t *) ptr + *(size_t *) block_front;
         }
    }
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */




void *put_left(void *ptr, int old_size, int new_size, int size_left){
    int copySize = old_size;
    memcpy(ptr - (new_size - old_size), ptr, copySize);
    *(size_t *) (ptr -  2 * SIZE_T_SIZE + old_size) = *(size_t *) (ptr - SIZE_T_SIZE - (new_size - old_size)) = new_size | 1;
    *(size_t *) (ptr - SIZE_T_SIZE - size_left) = *(size_t *) (ptr - 2 * SIZE_T_SIZE - (new_size - old_size)) = size_left - new_size + old_size;
    return ptr - (new_size - old_size);
}

void *put_right(void *ptr, int old_size, int new_size, int size_right){
    *(size_t *) (ptr - 2 * SIZE_T_SIZE + new_size) =  *(size_t *) (ptr - 1 * SIZE_T_SIZE) = new_size | 1;
    *(size_t *) (ptr - SIZE_T_SIZE + new_size) = *(size_t *)(ptr - 2 * SIZE_T_SIZE + old_size + size_right) = size_right - new_size + old_size;
    return ptr;
}



void *mm_realloc(void *ptr, size_t size){

    size_t new_size = ALIGN(size + 2 * SIZE_T_SIZE);
    size_t old_size = (*(size_t *)(ptr - SIZE_T_SIZE)) & -2;
    //CASE1
    if(ptr == NULL){
        return mm_malloc(size);
    }
    //CASE2 - same size
    if(new_size == old_size){
        return ptr;
    }

    //CASE3 - smaller
    if(new_size < old_size){
        if(new_size + 1 * SIZE_T_SIZE == old_size){
        }else{
            //printf("1:%p %d   2:%p %d   3:%p %d    4:%p %d  \n", ptr - SIZE_T_SIZE, *(size_t *) (ptr - SIZE_T_SIZE), ptr - 2 * SIZE_T_SIZE + new_size, *(size_t *)(ptr - 2 * SIZE_T_SIZE + new_size), ptr - SIZE_T_SIZE + new_size, *(size_t *)( ptr - SIZE_T_SIZE + new_size), ptr - 2 * SIZE_T_SIZE + new_size + *(size_t *)(ptr - 2 * SIZE_T_SIZE + new_size), *(size_t*)(ptr - 2 * SIZE_T_SIZE + new_size + *(size_t *)(ptr - 2 * SIZE_T_SIZE + new_size)));
            *(size_t *)(ptr - 2 * SIZE_T_SIZE + old_size) = *(size_t *)(ptr - SIZE_T_SIZE + new_size) = (new_size - old_size) | 1;
            *(size_t *) (ptr - SIZE_T_SIZE) = *(size_t *)(ptr- 2 * SIZE_T_SIZE + new_size) = new_size| 1;
            //printf("1:%p %d   2:%p %d   3:%p %d    4:%p %d  \n", ptr - SIZE_T_SIZE, *(size_t *) (ptr - SIZE_T_SIZE), ptr - 2 * SIZE_T_SIZE + new_size, *(size_t *)(ptr - 2 * SIZE_T_SIZE + new_size), ptr - SIZE_T_SIZE + new_size, *(size_t *)( ptr - SIZE_T_SIZE + new_size), ptr - 2 * SIZE_T_SIZE + new_size + *(size_t *)(ptr - 2 * SIZE_T_SIZE + new_size), *(size_t*)(ptr - 2 * SIZE_T_SIZE + new_size + *(size_t *)(ptr - 2 * SIZE_T_SIZE + new_size)));
            free(ptr + new_size);
            return ptr;
        }
    }
    //CASE4 - new-SIZE > old_size;

    size_t size_left = *(size_t *)(ptr - 2 * SIZE_T_SIZE);
    size_t size_right = *(size_t *)(ptr - SIZE_T_SIZE + old_size);



    if(ptr - 2 *SIZE_T_SIZE > mem_heap_lo() && (size_left & 1) == 0 && size_left >= new_size - old_size + 2 * SIZE_T_SIZE){ 
        if(ptr + old_size < mem_heap_hi() && (size_right & 1) == 0 && size_right >= new_size - old_size + 2 * SIZE_T_SIZE){ //both possible (left and right)
             if(size_left < size_right){ //best fit, take the smallest one if possible
                return put_left(ptr, old_size, new_size, size_left);
             }else{
                return put_right(ptr, old_size, new_size, size_right);
             }
        }else{ //just left possible
             return put_left(ptr, old_size, new_size, size_left);
        }
    }

    //just right possible
    if(ptr + old_size < mem_heap_hi() && (size_right & 1) == 0 && size_right >= new_size - old_size + 2 * SIZE_T_SIZE){ // just right
           return put_right(ptr, old_size, new_size, size_right);
    }
    //not left, not right case
        void *oldptr = ptr;
        void *newptr;
        size_t copySize;
        
        newptr = mm_malloc(size);
        if (newptr == NULL)
        return NULL;
        copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
        if (size < copySize)
        copySize = size;
        memcpy(newptr, oldptr, copySize);
        mm_free(oldptr);
        return newptr;
}