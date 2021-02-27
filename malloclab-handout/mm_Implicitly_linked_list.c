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

static void *extend_heap(size_t words);
static void *coalesce(void *ptr);
static void place(void *bp, size_t asize);

static char *heap_listp;
static char *pre_listp;

static void* extend_heap(size_t words){
    char* bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));   // head
    PUT(FTRP(bp), PACK(size, 0));   // footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));   // new epilogue header

    return coalesce(bp);
}


static void *coalesce(void *ptr){
    size_t size = GET_SIZE(HDRP(ptr));
    size_t is_prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(ptr)));
    size_t is_next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));

    if (is_prev_alloc && is_next_alloc){
        return ptr;
    } else if (is_prev_alloc && !is_next_alloc){ // next
        size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(ptr), PACK(size, 0));
        PUT(FTRP(ptr), PACK(size, 0));
    } else if (!is_prev_alloc && is_next_alloc){ // pre
        size += GET_SIZE(HDRP(PREV_BLKP(ptr)));
        PUT(FTRP(ptr), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        ptr = PREV_BLKP(ptr);
    } else { //all
        size += GET_SIZE(HDRP(PREV_BLKP(ptr))) + GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(size, 0));
        ptr = PREV_BLKP(ptr);
    }

    // for next_fit
    if (pre_listp > ptr && pre_listp < NEXT_BLKP(ptr))
            pre_listp = ptr;
    return ptr;
}


static void place(void *bp, size_t asize){
    size_t remain_size;
    remain_size = GET_SIZE(HDRP(bp)) - asize;
    //如果剩余空间满足最小块大小，就将其作为一个新的空闲块
    if(remain_size >= DSIZE){
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(remain_size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(remain_size, 0));
    }else{
        PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
        PUT(FTRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
    }
}

// static void *first_fit(size_t asize){
//     void *bp = heap_listp;
//     size_t size;
//     while((size = GET_SIZE(HDRP(bp))) != 0){    //遍历全部块
//         if(size >= asize && !GET_ALLOC(HDRP(bp)))    //寻找大小大于asize的空闲块
//             return bp;
//         bp = NEXT_BLKP(bp);
//     }
//     return NULL;
// } 

static void *next_fit(size_t asize){
    for (char* bp = pre_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize)
        {
            pre_listp = bp;
            return bp;
        }
    }

    for (char* bp = heap_listp; bp != pre_listp; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize)
        {
            pre_listp = bp;
            return bp;
        }
    }
    return NULL;
}

// static void *best_fit(size_t asize){
//     void *bp = heap_listp;
//     size_t size;
//     void *best = NULL;
//     size_t min_size = 0;
    
//     while((size = GET_SIZE(HDRP(bp))) != 0){
//         if(size >= asize && !GET_ALLOC(HDRP(bp)) && (min_size == 0 || min_size>size)){   //记录最小的合适的空闲块
//             min_size = size;
//             best = bp;
//         }
//         bp = NEXT_BLKP(bp);
//     }
//     return best;
// } 


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0); //padding
    PUT(heap_listp+1*WSIZE, PACK(DSIZE, 1));    //head
    PUT(heap_listp+2*WSIZE, PACK(DSIZE, 1));    //footer
    PUT(heap_listp+3*WSIZE, PACK(0, 1));
    
    heap_listp += DSIZE;    //指向序言块有效载荷的指针
    pre_listp = heap_listp;
    if(extend_heap(CHUNKSIZE/WSIZE) == NULL)    //申请更多的堆空间
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size){
    size_t asize; // adjusted block size
    size_t extendsize; // amount to extend heap if no fit
    char *bp;

    // count size first
    if (size == 0)
        return NULL;

    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    if ((bp = next_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    size = GET_SIZE(HDRP(oldptr));
    copySize = GET_SIZE(HDRP(newptr));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize-WSIZE);
    mm_free(oldptr);
    return newptr;
}














