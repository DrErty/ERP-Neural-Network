#include "RingBuffer.h"

void RingBuffer::Push(Scalar value)
{
    m_Data[m_Head] = value;
    m_Head = (m_Head + 1) % CAPACITY;
    if (m_Count < CAPACITY)
        ++m_Count;
}

void RingBuffer::Push(std::span<const Scalar> values)
{
    for (Scalar v : values)
        Push(v);
}

void RingBuffer::Clear()
{
    m_Head = 0;
    m_Count = 0;
}

std::span<const Scalar> RingBuffer::Span()
{
    if (m_Count == CAPACITY && m_Head != 0)
    {
        std::rotate(m_Data.begin(), m_Data.begin() + m_Head, m_Data.end());
        m_Head = 0;
    }
    return std::span<const Scalar>(m_Data.data(), m_Count);
}
