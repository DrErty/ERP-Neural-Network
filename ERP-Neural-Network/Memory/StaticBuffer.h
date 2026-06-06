#pragma once

#include <cstring>

#include "DynamicAllocator.h"
#include "Iterator.h"
#include "StackAllocator.h"

template<typename T>
class StaticBuffer
{
public:
	static constexpr bool UseStack = false;

	explicit StaticBuffer(size_t count)
	{
		Allocate(count);
	}

	explicit StaticBuffer()
		: m_Data(nullptr), m_Count(0) {}

	StaticBuffer(const StaticBuffer&) = delete;
	StaticBuffer& operator=(const StaticBuffer&) = delete;
	StaticBuffer(StaticBuffer&& other) noexcept
		: m_Data(other.m_Data), m_Count(other.m_Count)
	{
		other.m_Data = nullptr;
	}

	StaticBuffer& operator=(StaticBuffer&&) = delete;

	~StaticBuffer()
	{
		for (size_t i = m_Count; i >= 1; i--)
			m_Data[i - 1].~T();

		if constexpr (UseStack)
			Memory::ThreadStackAllocator.Deallocate(m_Data, m_Count * sizeof(T), alignof(T));
		else
			DynamicAllocator::Deallocate(m_Data, m_Count * sizeof(T));
	}

	Iterator<T> begin() { return Iterator<T>(&m_Data[0]); }
	Iterator<T> end() { return Iterator<T>(&m_Data[m_Count]); }

	Iterator<const T> begin() const { return Iterator<const T>(&m_Data[0]); }
	Iterator<const T> end() const { return Iterator<const T>(&m_Data[m_Count]); }

	T* GetData() { return m_Data; }
	const T* GetData() const { return m_Data; }

	T& GetFirst() { return m_Data[0]; }
	T& GetLast() { return m_Data[m_Count - 1]; }

	const T& GetFirst() const { return m_Data[0]; }
	const T& GetLast() const { return m_Data[m_Count - 1]; }

	size_t GetCount() const { return m_Count; }

	T& operator[](size_t index)
	{
		//ERP_ASSERT(index < m_Count, "Index out of range");
		return m_Data[index];
	}

	const T& operator[](size_t index) const
	{
		//ERP_ASSERT(index < m_Count, "Index out of range");
		return m_Data[index];
	}

	void Allocate(size_t count)
	{
		m_Count = count;
		if constexpr (UseStack)
			m_Data = static_cast<T*>(Memory::ThreadStackAllocator.Allocate(count * sizeof(T), alignof(T)));
		else
			m_Data = static_cast<T*>(DynamicAllocator::Allocate(count * sizeof(T)));

		for (size_t i = 0; i < m_Count; i++)
			new(&m_Data[i]) T;
	}
private:
	T* m_Data;
	size_t m_Count;
};