// ReSharper disable CppClangTidyClangDiagnosticImplicitExceptionSpecMismatch
// ReSharper disable CppParameterNamesMismatch

#include "new.h"

#pragma warning( push )
#pragma warning( disable : 28251) // Inconsistent SAL annotation

#ifdef AM_USE_SYS_ALLOCATORS

void* operator new(size_t size, size_t align, rage::eAllocatorType type)
{
	return GetMultiAllocator()->Allocate(size, align, type);
}

void* operator new(size_t size)
{
	return GetMultiAllocator()->Allocate(size);
}

void* operator new(size_t size, size_t align)
{
	return GetMultiAllocator()->Allocate(size, align);
}

void* operator new [](size_t size)
{
	return GetMultiAllocator()->Allocate(size);
}

void* operator new [](size_t size, size_t align)
{
	return GetMultiAllocator()->Allocate(size, align);
}

void operator delete(void* block)
{
	if (!block)
		return;
	GetMultiAllocator()->Free(block);
}

void operator delete [](void* block)
{
	if (!block)
		return;
	GetMultiAllocator()->Free(block);
}

#endif

#pragma warning( pop ) 
