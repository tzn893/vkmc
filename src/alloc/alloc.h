#pragma once
#include "common.h"
#include <new>
//memory allocator can only allocate memory
class Allocator
{
public:
	virtual void* Allocate(size_t size) = 0;

	template<typename T,typename ...Args>
	T* Create(Args... args)
	{
		void* obj = Allocate(sizeof(T));
		return new (obj) T (args...);
	}
};

//memory manager can not only allocate but also deallocate memory
class MemoryManager : public Allocator
{
public:
	virtual void Deallocate(void* ptr) = 0;

	template<typename T>
	void Destroy(T*& obj)
	{
		Deallocate(obj);
		obj = nullptr;
	}
};