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

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

//字大小和双字大小
#define WSIZE 4
#define DSIZE 8
//当堆内存不够时，向内核申请的堆空间
#define CHUNKSIZE (1<<12)
//将val放入p开始的4字节中
#define PUT(p,val) (*(unsigned int*)(p) = (val))
//获得头部和脚部的编码
#define PACK(size, alloc) ((size) | (alloc))
//从头部或尾部获得块大小和已分配位
#define GET_SIZE(p) (*(unsigned int*)(p) & ~0x7)
#define GET_ALLOC(p) (*(unsigned int*)(p) & 0x1)
//获得块的头部和尾部
#define HDRP(bp) ((char*)(bp) - WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
//获得上一个块和下一个块
#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE((char*)(bp) - DSIZE))
#define MAX(x,y) ((x)>(y)?(x):(y))
//获得块中记录后继和前驱的地址
#define FD(ptr) (*(char **)(ptr))
#define BK(ptr) (*(char **)(BK_PTR(ptr)))

#define FD_PTR(ptr) ((char *)(ptr))
#define BK_PTR(ptr) ((char *)(ptr) + WSIZE)
#define SET_PTR(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))
#define ListMax 16

static void *heap_extension(size_t size);
static void *coalesce(void *ptr);
static void insert_node(void *ptr, size_t size);
static void delete_node(void *ptr);
void *free_lists[ListMax];

static void *heap_extension(size_t size){
    void *ptr;
    size = ALIGN(size);
    if ((ptr = mem_sbrk(size)) == (void *)-1)
        return NULL;

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    
    PUT(HDRP(NEXT_BLKP(ptr)), PACK(0, 1));
    
    insert_node(ptr, size);
    
    return coalesce(ptr);
}

static void insert_node(void *ptr, size_t size){
    int i = 0;
    void *search = NULL;
    void *insert = NULL;

    while ((i < 15) && (size > 1)){
        size >>= 1;
        i++;
    }

    search = free_lists[i];
    while ((search != NULL) && (size > GET_SIZE(HDRP(search)))){
        insert = search;
        search = FD(search);
    }

    if (search != NULL){
        if (insert != NULL){
            // insert < ptr < search
            SET_PTR(FD_PTR(ptr), search);
            SET_PTR(BK_PTR(ptr), insert);
            SET_PTR(FD_PTR(insert), ptr);
            SET_PTR(BK_PTR(search), ptr);
        }
        else{
            // place ptr at first
            SET_PTR(FD_PTR(ptr), search);
            SET_PTR(BK_PTR(search), ptr);
            SET_PTR(BK_PTR(ptr), NULL);
            free_lists[i] = ptr;
        }
    }
    else{
        if (insert != NULL){ 
            // place ptr at last
            SET_PTR(FD_PTR(ptr), NULL);
            SET_PTR(BK_PTR(ptr), insert);
            SET_PTR(FD_PTR(insert), ptr);
        }
        else{ 
            // bin is free
            SET_PTR(FD_PTR(ptr), NULL);
            SET_PTR(BK_PTR(ptr), NULL);
            free_lists[i] = ptr;
        }
    }
}

// unlink
static void delete_node(void *ptr)
{
    int i = 0;
    size_t size = GET_SIZE(HDRP(ptr));
    while ((i < 15) && (size > 1)){
        size >>= 1;
        i++;
    }
    if (FD(ptr) != NULL){
        if (BK(ptr) != NULL){
            SET_PTR(BK_PTR(FD(ptr)), BK(ptr));
            SET_PTR(FD_PTR(BK(ptr)), FD(ptr));
        }
        else{
            SET_PTR(BK_PTR(FD(ptr)), NULL);
            free_lists[i] = FD(ptr);
        }
    }
    else{
        if (BK(ptr) != NULL){
            SET_PTR(FD_PTR(BK(ptr)), NULL);
        }
        else{
            free_lists[i] = NULL;
        }
    }
}

static void* place(void* ptr, size_t size) {
    size_t ptr_size = GET_SIZE(HDRP(ptr));
    size_t remainder = ptr_size - size;

    delete_node(ptr);

    if (remainder < DSIZE * 2) {
        PUT(HDRP(ptr), PACK(ptr_size, 1));
        PUT(FTRP(ptr), PACK(ptr_size, 1));
    }
    else {
        PUT(HDRP(ptr), PACK(size, 1));
        PUT(FTRP(ptr), PACK(size, 1));
        PUT(HDRP(NEXT_BLKP(ptr)), PACK(remainder, 0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(remainder, 0));
        insert_node(NEXT_BLKP(ptr), remainder);
    }
    return ptr;
}

static void *first_fit(size_t size){
    size_t size_temp = size;
    void *ptr = NULL;

    for(int i = 0;i < ListMax;i++){
        if (((size_temp <= 1) && (free_lists[i] != NULL))){
            ptr = free_lists[i];
            while ((ptr != NULL) && ((size > GET_SIZE(HDRP(ptr))))){
                ptr = FD(ptr);
            }
            if (ptr != NULL)
                return ptr;
        }
        size_temp >>= 1;
    }
    return NULL;
} 

static void *coalesce(void *ptr){
    size_t size = GET_SIZE(HDRP(ptr));
    size_t is_prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(ptr)));
    size_t is_next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    if (is_prev_alloc && is_next_alloc){
        return ptr;
    }
    else if (is_prev_alloc && !is_next_alloc){
        delete_node(ptr);
        delete_node(NEXT_BLKP(ptr));
        size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(ptr), PACK(size, 0));
        PUT(FTRP(ptr), PACK(size, 0));
    }
    else if (!is_prev_alloc && is_next_alloc){
        delete_node(ptr);
        delete_node(PREV_BLKP(ptr));
        size += GET_SIZE(HDRP(PREV_BLKP(ptr)));
        PUT(FTRP(ptr), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        ptr = PREV_BLKP(ptr);
    }
    else{
        delete_node(ptr);
        delete_node(PREV_BLKP(ptr));
        delete_node(NEXT_BLKP(ptr));
        size += GET_SIZE(HDRP(PREV_BLKP(ptr))) + GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(size, 0));
        ptr = PREV_BLKP(ptr);
    }
    insert_node(ptr, size);
    return ptr;
}


int mm_init(void){
    char *heap; 
    for (int i =0 ; i < ListMax; i++)
        free_lists[i] = NULL;

    if ((long)(heap = mem_sbrk(4*WSIZE)) == -1)
        return -1; 
          
    PUT(heap, 0);
    PUT(heap + 4, 9);
    PUT(heap + 8, 9);
    PUT(heap + 12, 1);
    
    if (heap_extension(64) == NULL)
        return -1;
    return 0;
}


void *mm_malloc(size_t size){
    void *ptr = NULL;
    if (size == 0)
        return NULL;
    if (size <= 8)
        size = 16;
    else
        size = ALIGN(size + 8);

    if (((ptr = first_fit(size))) == NULL){
        if ((ptr = heap_extension(MAX(size, CHUNKSIZE))) == NULL)
            return NULL;
    }

    place(ptr, size);
    return ptr;
}


void mm_free(void *ptr){
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    
    insert_node(ptr, size);  
    coalesce(ptr);
}

void *mm_realloc(void *ptr, size_t size){
    void *nblock = ptr;
    int reamin;
    if (size == 0)
        return NULL;
    if (size <= 8){
        size = 16;
    }
    else{
        size = ALIGN(size + 8);
    }
    if ((reamin = GET_SIZE(HDRP(ptr)) - size) >= 0){
        return ptr;
    }
    else if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) || !GET_SIZE(HDRP(NEXT_BLKP(ptr)))){
        if ((reamin = GET_SIZE(HDRP(ptr)) + GET_SIZE(HDRP(NEXT_BLKP(ptr))) - size) < 0){
            if (heap_extension(MAX(-reamin, CHUNKSIZE)) == NULL)
                return NULL;
            reamin += MAX(-reamin, CHUNKSIZE);
        }
        delete_node(NEXT_BLKP(ptr));
        PUT(HDRP(ptr), PACK(size + reamin, 1));
        PUT(FTRP(ptr), PACK(size + reamin, 1));
    }
    else{
        nblock = mm_malloc(size);
        memcpy(nblock, ptr, GET_SIZE(HDRP(ptr)));
        mm_free(ptr);
    }
    return nblock;
}

