#pragma once

#include "ERP.h"

#include "StackAllocator.h"

template<typename T, size_t C>
class ObjectPoolAllocator
{
public:
	ObjectPoolAllocator()
	{
		m_Blocks = static_cast<Block*>(Memory::ThreadStackAllocator.Allocate(C * sizeof(Block), alignof(Block)));

		m_Head = &m_Blocks[0];
		m_Blocks[C - 1].Next = nullptr;
		for (size_t i = 0; i < (C - 1); i++)
			m_Blocks[i].Next = &m_Blocks[i + 1];
	}

	ObjectPoolAllocator(const ObjectPoolAllocator&) = delete;
	ObjectPoolAllocator& operator=(const ObjectPoolAllocator&) = delete;
	ObjectPoolAllocator(ObjectPoolAllocator&&) = delete;
	ObjectPoolAllocator& operator=(ObjectPoolAllocator&&) = delete;

	~ObjectPoolAllocator()
	{
		Memory::ThreadStackAllocator.Deallocate(m_Blocks, C * sizeof(Block), alignof(Block));
	}

	template<typename... Args>
	T* Allocate(Args&&... args)
	{
		Block* const block = m_Head;
		if (block == nullptr)
		{
			ERP_EXIT_FATAL("Pool allocator full.");
			return nullptr;
		}
		m_Head = block->Next;

		T* const data = reinterpret_cast<T*>(block->Data);
		new(data) T(std::forward<Args>(args)...);
		return data;
	}

	void Deallocate(T* data)
	{
		data->~T();
		Block* const block = reinterpret_cast<Block*>(data);
		block->Next = m_Head;
		m_Head = block;
	}
private:
	union Block
	{
		alignas(T) char Data[sizeof(T)];
		Block* Next;
	};
private:
	Block* m_Head;
	Block* m_Blocks;
};