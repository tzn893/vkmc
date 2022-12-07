#pragma once
#include "common.h"

//memory allocator can only allocate memory
class Allocator
{
public:
	virtual void* Allocate(size_t size) = 0;
};

//memory manager can not only allocate but also deallocate memory
class MemoryManager : public Allocator
{
public:
	virtual void Deallocate(void* ptr) = 0;
};