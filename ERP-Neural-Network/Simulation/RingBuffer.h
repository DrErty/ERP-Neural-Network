#pragma once

#include "ERP.h"

class RingBuffer
{
public:
    static constexpr std::size_t CAPACITY = 1024 * 4;

    RingBuffer();

    void Push(Scalar value);
    void Push(std::span<const Scalar> values);
    void Clear();

    std::size_t Size() const { return m_Count; }
    bool Empty() const { return m_Count == 0; }
    bool Full() const { return m_Count == CAPACITY; }

    std::span<const Scalar> Span();
private:
    StaticBuffer<Scalar> m_Data;
    std::size_t m_Head = 0;
    std::size_t m_Count = 0;
};

