#ifndef RING_BUFFER_HPP_
#define RING_BUFFER_HPP_

#include <stddef.h>
#include <stdint.h>

template<typename T, size_t count>
class ring_buffer
{
private:
	size_t head_ = 0;
	size_t tail_ = 0;
	bool full_ = 0;
	T buffer_[count];
	
public:
	ring_buffer() = default;
	
	void put(T data) noexcept
	{
		buffer_[head_] = data;
		
		if (full_)
		{
			tail_ = (tail_ + 1) % count;
		}
		head_ = (head_ + 1) % count;
		
		full_ = head_ == tail_;
	}
	
	T get() noexcept
	{
		auto value = buffer_[tail_];
		full_ = false;
		tail_ = (tail_ + 1) % count;
		return value;
	}
	
	void reset() noexcept
	{
		head_ = tail_;
		full_ = false;
	}
	
	bool empty() const noexcept
	{
		return (!full_ && (head_ == tail_));
	}
	
	bool full() const noexcept
	{
		return full_;
	}
	
	size_t capacity() const noexcept
	{
		return count;
	}
	
	size_t size() const noexcept
	{
		size_t size = count;
		
		if (!full_)
		{
			if (head_ >= tail_)
			{
				size = head_ - tail_;
			}
			else
			{
				size = count + head_ - tail_;
			}
		}
		return size;
	}
	
};


#endif
