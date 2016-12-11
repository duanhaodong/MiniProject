/*
 * mm.c
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */


/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/*
 * If NEXT_FIT defined use next fit search, else use first-fit search 
 */
#define NEXT_FITx

/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */ 
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */  

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* Pack a size and allocated bit into a word */
#define U(bp) ((bp<zero?zero+(unsigned)bp:bp))
#define PACK(size, alloc)  ((size) | (alloc)) 

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int*)(U(p)))           
#define PUTTRUNC(p, val)  (GET(p) = (val))    
#define PUT(p,val) (GET(p) = (GET(p) & 0x2) | (unsigned) (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)                   
#define GET_ALLOC(p) (GET(p) & 0x1)                    

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(U(bp)) - WSIZE)                      
#define FTRP(bp)       ((char *)(U(bp)) + GET_SIZE(HDRP(bp)) - DSIZE) 

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  (unsigned*)((char *)(U(bp)) + GET_SIZE(((char *)(U(bp)) - WSIZE))) 
#define PREV_BLKP(bp)  (unsigned*)((char *)(U(bp)) - GET_SIZE(((char *)(U(bp)) - DSIZE))) 
#define ALLOC_NEXT(bp)  (GET(HDRP(NEXT_BLKP(U(bp))))|=2)
#define UNALLOC_NEXT(bp) (GET(HDRP(NEXT_BLKP(U(bp))))&=~2)
#define PREV(bp) (*(unsigned *)(U((char*)bp)))
#define NEXT(bp) (*(unsigned *)(U((char*)bp+4)))
#define LCH(bp) (*(unsigned *)(U((char*)bp+8)))
#define RCH(bp) (*(unsigned *)(U((char*)bp+12)))
#define PAR(bp) (*(unsigned *)(U((char*)bp+16)))

/* Global variables */
static char *heap_listp = 0;  /* Pointer to first block */  
static char *zero = 0x800000000; 
static unsigned* list;
static unsigned* root;

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static unsigned *find_fit(size_t asize);
static void *coalesce(void *bp);
static int aligned(const void *p);
static int in_heap(const void *p);
unsigned* bestfit(unsigned *node, size_t size);

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)) & ~0x7)

/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void) {
	//freopen("out.txt","w",stdout);
	if ((heap_listp = mem_sbrk(40+4*WSIZE)) == (void *)-1) 
		return -1;
	list = (unsigned*)heap_listp;
	heap_listp+=40;
	root=heap_listp;
	memset(list,0,40);
	PUTTRUNC(heap_listp, 0);                          /* Alignment padding */
	PUTTRUNC(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */ 
	PUTTRUNC(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */ 
	PUTTRUNC(heap_listp + (3*WSIZE), PACK(0, 1));     /* Epilogue header */
	heap_listp += DSIZE;
	ALLOC_NEXT(heap_listp);  
    	return 0;
}

/*
 * malloc
 */
void *malloc (size_t size) {
	//Print_list();
	//Print_tree(*root); 
	size_t asize;      /* Adjusted block size */
	size_t extendsize; /* Amount to extend heap if no fit */
	char *bp;      
	
	if (heap_listp == 0){
		mm_init();
	}
	/* Ignore spurious requests */
	if (size == 0)
		return NULL;

	/* Adjust block size to include overhead and alignment reqs. */
	if (size <= DSIZE)                                          
		asize = 4*DSIZE;                                        
	else
		asize = DSIZE * ((size + (WSIZE*6) + (DSIZE-1)) / DSIZE); 
	//printf("a%d\n",asize);
	/* Search the free list for a fit */
	
	if ((bp = find_fit(asize)) != NULL) {  
		//printf("a%d %d\n",asize,bp);
		place(bp, asize);   
		//Print_tree(*root);            
		return U(bp);
	}
	
	/* No fit found. Get more memory and place the block */
	extendsize = MAX(asize,CHUNKSIZE);    
    //printf("flag\n");
	if ((bp = extend_heap(extendsize/WSIZE)) == NULL)  
		return NULL;   
	//printf("a%d %d\n",asize,bp);  
	//Print_tree(&root);                           
	place(bp, asize);                                 
	return U(bp);
}

/*
 * free
 */
void free (void *ptr) {
	//printf("b%d\n",ptr);
	if (ptr == 0||!aligned(ptr)||!in_heap(ptr)) 
		return;

	size_t size = GET_SIZE(HDRP(ptr));
	if (heap_listp == 0){
		mm_init();
	}

	PUT(HDRP(ptr), PACK(size, 0));
	PUT(FTRP(ptr), PACK(size, 0));
	UNALLOC_NEXT(ptr);
	void* tmp = coalesce(ptr);    
	insert(tmp, GET_SIZE(HDRP(tmp)));  
}

/*
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *oldptr, size_t size) {
	//printf("c%d\n",size);
	size_t oldsize;
	void *newptr;

	/* If size == 0 then this is just free, and we return NULL. */
	if(size == 0) {
		mm_free(oldptr);
		return 0;
	}

	/* If oldptr is NULL, then this is just malloc. */
	if(oldptr == NULL) {
		return mm_malloc(size);
	}

	newptr = mm_malloc(size);

	/* If realloc() fails the original block is left untouched  */
	if(!newptr) {
		return 0;
	}

	/* Copy the old data. */
	oldsize = GET_SIZE(HDRP(oldptr));
	if(size < oldsize) oldsize = size;
	memcpy(newptr, oldptr, oldsize);

	/* Free the old block. */
	free(oldptr);

	return newptr;
}

/*
 * calloc - you may want to look at mm-naive.c
 * This function is not tested by mdriver, but it is
 * needed to run the traces.
 */
void *calloc (size_t nmemb, size_t size) {
	//printf("d%d\n",size);
	size_t bytes = nmemb * size;
	void *newptr;

	newptr = malloc(bytes);
	memset(newptr, 0, bytes);

	return newptr;
}


/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static int in_heap(const void *p) {
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static int aligned(const void *p) {
    return (size_t)ALIGN(p) == (size_t)p;
}

/*
 * mm_checkheap
 */
void mm_checkheap(int lineno) {
	lineno=lineno;
}

/* 
 * extend_heap - Extend heap with free block and return its block pointer
 */
static void *extend_heap(size_t words) 
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; 
    if ((long)(bp = mem_sbrk(size)) == -1)  
        return NULL;                                        

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */   
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */   
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ 

    /* Coalesce if the previous block was free */
    return coalesce(bp);                                          
}

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
static void *coalesce(void *bp) 
{
    size_t size = GET_SIZE(HDRP(bp));
	unsigned prev=PREV_BLKP(bp),next=NEXT_BLKP(bp);
    size_t prev_alloc = GET_ALLOC(FTRP(prev));
    size_t next_alloc = GET_ALLOC(HDRP(next));

    if (prev_alloc && next_alloc) {            /* Case 1 */
        return bp;
    }

    else if (prev_alloc && !next_alloc) {      /* Case 2 */
    	erase(next);
        size += GET_SIZE(HDRP(next));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
    	erase(prev);
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(prev), PACK(size, 0));
        bp = prev;
    }

    else {                                     /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
        erase(prev); erase(next);
        PUT(HDRP(prev), PACK(size, 0));
        PUT(FTRP(next), PACK(size, 0));
        bp = prev;
    }
    return bp;
}

/* 
 * place - Place block of asize bytes at start of free block bp 
 *         and split if remainder would be at least minimum block size
 */
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));   

    if ((csize - asize) >= (4*DSIZE)) { 
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        ALLOC_NEXT(bp);
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
        UNALLOC_NEXT(bp);
        //printf("%lld %lld\n",bp,GET_SIZE(HDRP(bp)));
        //printf("flagggg\n");
        insert(bp);
    }
    else { 
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
        ALLOC_NEXT(bp);
    }
}

/* 
 * find_fit - Find a fit for a block with asize bytes 
 */
static unsigned *find_fit(size_t asize){
    unsigned cur;
	unsigned* block;    
    size_t dcount = asize / DSIZE; 
    if(dcount<=13){
    	if(list[dcount-4]){
    		cur=list[dcount-4];
    		list[dcount-4]=NEXT(cur);
    		erase(cur);
    		return cur;
		}
	}
	block=bestfit(root, asize);
    if (block == NULL) 
        return NULL;    
    cur = *block;
	//printf("%lld %lld\n",cur,GET_SIZE(HDRP(cur)));  
    erase(cur);    
    
    return cur;   
}

void insert(void* bp){
	NEXT(bp)=PREV(bp)=LCH(bp)=RCH(bp)=PAR(bp)=0;
	int d=GET_SIZE(HDRP(bp));
	d/=8;
	if(d<=13){
		unsigned cur_n=(unsigned)list[d-4];
		list[d-4]=(unsigned)bp;
		if(cur_n){
			void* ptr_n=(void *)cur_n;
			NEXT(bp)=cur_n;
			PREV(ptr_n)=(unsigned)bp;
		}
		return;
	}
	unsigned* p=root;
	void* parent=NULL;
	while(GET(p)){
		parent=GET(p);
		size_t c_size=GET_SIZE(HDRP(parent));
		//printf("haha%lld %lld\n",c_size,GET_SIZE(HDRP(bp)));
		if(GET_SIZE(HDRP(bp))<c_size){
			p=&LCH(parent);
		} else if (GET_SIZE(HDRP(bp))>c_size){
			p=&RCH(parent);
		} else {
			unsigned nxt=NEXT(parent);
			NEXT(bp)=nxt;
			if(nxt){
				PREV(nxt)=bp;
			}
			NEXT(parent)=bp;
			PREV(bp)=parent;
			return;
		}
	}
	PUT(p,bp);
	PAR(bp)=p;
}

void erase(void* bp){
	size_t sz=GET_SIZE(HDRP(bp));
	sz/=8;
	if(sz<=13){
		if(!PREV(bp))
			list[sz-4]=NEXT(bp);
		unsigned prev=PREV(bp),next=NEXT(bp);
		if(prev){
			NEXT(prev)=next;
		}
		if(next){
			PREV(next)=prev;
		}
	} else {
		//printf("%lld %lld\n",bp,PAR(bp));
		if(PAR(bp)){
			unsigned l=LCH(bp),r=RCH(bp);
			//printf("%lld %lld %lld\n",l,r,PAR(bp));
			if(NEXT(bp)){
				PAR(NEXT(bp))=PAR(bp);
				PUT(PAR(bp),NEXT(bp));
				unsigned cur=NEXT(bp);
				LCH(cur)=l; RCH(cur)=r;
				if(l)	PAR(l)=&LCH(cur);
				if(r)	PAR(r)=&RCH(cur);
			} else if(l&&!r){
				PUT(PAR(bp),l);
				PAR(l)=PAR(bp);
			} else if(r&&!l){
				PUT(PAR(bp),r);
				PAR(r)=PAR(bp);
			} else if(l&&r){
				unsigned cur=r;
				if(!LCH(r)){
					LCH(r)=l;
					PAR(r)=PAR(bp);
					PUT(PAR(bp),r);
					PAR(l)=&LCH(r);
				} else {
					while(LCH(cur)){
						cur=LCH(cur);
					}
					PUT(PAR(cur),RCH(cur));
					if(RCH(cur))
						PAR(RCH(cur))=PAR(cur);
					RCH(cur)=RCH(bp);
					PAR(RCH(bp))=&RCH(cur);
					LCH(cur)=LCH(bp);
					PAR(LCH(bp))=&LCH(cur);
					PAR(cur)=PAR(bp);
					PUT(PAR(bp),cur);
				}
			} else {
				PUT(PAR(bp),NULL);
			}
		} else {
			unsigned prev=PREV(bp),next=NEXT(bp);
			if(prev){
				NEXT(prev)=next;
			}
			if(next){
				PREV(next)=prev;
			}
		}
	}
}

unsigned* bestfit(unsigned *node, size_t size)    {    
    unsigned *candidate;
	void* curr = *node;   
    size_t curr_size;  
	
    if (curr == NULL){
	    return NULL;  
	}
    curr_size = GET_SIZE(HDRP(curr));  
	//printf("%lld %lld %lld\n",node,size,GET_SIZE(HDRP(curr)));
    if (size < curr_size)    
    {   
		//printf("%lld %lld\n",&LCH(curr),LCH(curr)) ;  
		
        if ((candidate = bestfit(&LCH(curr),size))!=NULL)    
            return candidate;   
		 
        return node;    
    }    
    else if (size > curr_size)    
        return bestfit(&RCH(curr), size);    
    else    
        return node;    
}   
void Print_list(){
	putchar('\n');
	for(int i=0;i<=9;i++){
		unsigned t=list[i];
		if(!t) continue;
		while(t){
			putchar(i+'0');
			putchar(':');
			printf("%lld %lld ",t,GET_SIZE(HDRP(t)));
			t=NEXT(t);
		}
		putchar('\n');
	}
}
void Print_tree(unsigned root){
	if(root){
		putchar('\n');
		unsigned cur=root;
		while(cur){
			printf("%d %d ",cur,GET_SIZE(HDRP(cur)));
			cur=NEXT(cur);
		}
		printf("/");
		Print_tree(LCH(root));
		printf("\\");
		Print_tree(RCH(root));
	}
} 
