#include "cache_alloc.h"

#include <iostream>
#include <stdlib.h>
#include <stdio.h>


CacheAllocer* MyCacheAlloc::CreateCacheAllocer(int capacity, int elem_size)
{
	if (elem_size<sizeof(Node))
	{
		elem_size = sizeof(Node);
	}
	CacheAllocer* cacheAllocer =(CacheAllocer*)malloc(sizeof(CacheAllocer));
	cacheAllocer->cache_mem = (unsigned char*)malloc(capacity*elem_size);
	memset(cacheAllocer->cache_mem, 0, capacity * elem_size);
	cacheAllocer->capacity = capacity;
	cacheAllocer->elem_size= elem_size;
	cacheAllocer->next = NULL;


	for (int i = 0; i < capacity; i++) {
		Node* walk = (Node*)(cacheAllocer->cache_mem + i * elem_size);
		walk->next = cacheAllocer->next;
		cacheAllocer->next = walk;
	}

	return cacheAllocer;
}


void* MyCacheAlloc::CacheAlloc(CacheAllocer* allocer, int elem_size)
{
	if (allocer->elem_size < elem_size) 
	{
		return malloc(elem_size);
	}

	if (allocer->next != NULL) //如果分配器还有剩余空间没有被分配	
	{                          //则返回一个空间
		void* now = allocer->next;
		allocer->next = allocer->next->next;
		return now;
	}
	return malloc(elem_size);//否则开辟一块新空间
}

void MyCacheAlloc::DestroyCacheAllocer(CacheAllocer* allocer)
{
	if (allocer->cache_mem!=NULL)
	{
		free(allocer->cache_mem);
	}

	free(allocer);
}

void MyCacheAlloc::CacheFree(CacheAllocer* allocer, void* mem)
{
	if (mem >= allocer->cache_mem && 
		mem <=allocer->cache_mem + allocer->capacity * allocer->elem_size)//如果该内存是分配器分配的则回收它否则直接释放
	{
		Node* node = (Node*)mem;
		node->next= allocer->next;
		allocer->next = node;
		return;
	}
	free(mem);
}
