#ifndef __CACHE_ALLOC_H__
#define __CACHE_ALLOC_H__


struct Node
{
	Node* next;
};

struct CacheAllocer
{
	int capacity;
	int elem_size;
	unsigned char* cache_mem;
	Node* next;
};

class MyCacheAlloc
{
	public:
		static CacheAllocer* CreateCacheAllocer(int capacity, int elem_size);
		static void DestroyCacheAllocer( CacheAllocer* allocer);
		static void* CacheAlloc( CacheAllocer* allocer, int elem_size);
		static void CacheFree( CacheAllocer* allocer, void* mem);
};

#endif // !__CACHE_ALLOC_H__
