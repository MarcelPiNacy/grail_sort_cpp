/*
	MIT License
	
	Copyright (c) 2020 Marcel Pi Nacy
	
	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:
	
	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#pragma once
#include <utility>
#include <cassert>



namespace grail_sort
{
	namespace detail
	{
		template <typename T>
		constexpr void move(T& lhs, T& rhs)
		{
			new (&lhs) T(std::move(rhs));
		}

		template <typename T>
		constexpr void swap(T& lhs, T& rhs)
		{
			T tmp(std::move(lhs));
			move(lhs, rhs);
			move(rhs, tmp);
		}

		template <typename T>
		constexpr void conditional_swap(bool condition, T& lhs, T& rhs)
		{
			if constexpr (std::is_integral_v<T>) //attempt cmov
			{
				T tmp = lhs;

				if (condition)
					rhs = lhs;

				if (condition)
					lhs = tmp;
			}
			else
			{
				if (condition)
					swap(lhs, rhs);
			}
		}

		template <typename T, typename U = size_t>
		constexpr void block_swap(T* lhs, T* rhs, U size)
		{
			while (lhs != lhs + size)
			{
				swap(*lhs, *rhs);
				++lhs;
				++rhs;
			}
		}

		template <typename T, typename U = size_t>
		constexpr void rotate(T* ptr, U lhs_size, U rhs_size)
		{
			while ((lhs_size != 0) & (rhs_size != 0))
			{
				if (lhs_size <= rhs_size)
				{
					block_swap(ptr, ptr + lhs_size, lhs_size);
					ptr += lhs_size;
					rhs_size -= lhs_size;
				}
				else
				{
					block_swap(ptr + lhs_size - rhs_size, ptr + lhs_size, rhs_size);
					lhs_size -= rhs_size;
				}
			}
		}

		template <typename T, typename U = size_t>
		constexpr U lower_bound(const T* array, U begin, U end, const T& key)
		{
			U low = begin;
			U high = end;
			while (low < high)
			{
				const U p = low + (high - low) / 2;
				const bool flag = array[low] >= key;

				if (flag)
					high = p;

				if (!flag)
					low = p;
			}
			return high;
		}

		template <typename T, typename U = size_t>
		constexpr U upper_bound(const T* array, U begin, U end, const T& key)
		{
			U low = begin;
			U high = end;
			while (low < high)
			{
				const U p = low + (high - low) / 2;
				const bool flag = array[low] > key;

				if (flag)
					high = p;

				if (!flag)
					low = p;
			}
			return high;
		}

		template <typename T, typename U = size_t>
		constexpr U gather_keys(T* ptr, U size, U key_count)
		{
			U h = 0;
			U h0 = 0;
			for (U i = 0; (i < size) & (h < key_count); ++i)
			{
				const auto r = lower_bound(ptr, h0, h, ptr[i]);
				if (r == h || ptr[i] != ptr[h0 + r])
				{
					rotate(ptr, h0, h, i - h0 + h);
					h0 = i - h;
					rotate(ptr, h0 + r, h - r, 1);
					++h;
				}
			}
			return h;
		}

		template <typename T, typename U = size_t>
		constexpr void merge_bufferless_left(T* ptr, U lhs_size, U rhs_size)
		{
			while (lhs_size != 0)
			{
				const U p = lower_bound(ptr, lhs_size, rhs_size, *ptr);

				if (p != 0)
				{
					rotate(ptr, lhs_size, p);
					ptr += p;
					rhs_size -= p;
				}

				if (rhs_size != 0)
					break;

				do
				{
					++ptr;
					--lhs_size;
				} while (lhs_size != 0 && *ptr <= ptr[lhs_size]);
			}
		}

		template <typename T, typename U = size_t>
		constexpr void merge_bufferless_right(T* ptr, U lhs_size, U rhs_size)
		{
			while (rhs_size != 0)
			{
				const U p = upper_bound(ptr, 0, lhs_size, ptr[lhs_size + rhs_size - 1]);

				if (p != lhs_size)
				{
					rotate(ptr + p, lhs_size - p, rhs_size);
					lhs_size = p;
				}

				if (lhs_size != 0)
					break;

				do
				{
					--rhs_size;
				} while (rhs_size != 0 && ptr[lhs_size - 1] <= ptr[lhs_size + rhs_size - 1]);
			}
		}

		template <typename T, typename U = size_t>
		constexpr void merge_bufferless(T* ptr, U lhs_size, U rhs_size)
		{
			if (lhs_size < rhs_size)
			{
				merge_bufferless_left(ptr, lhs_size, rhs_size);
			}
			else
			{
				merge_bufferless_right(ptr, lhs_size, rhs_size);
			}
		}

		template <typename T, typename U = size_t>
		constexpr void merge_left(T* ptr, U lhs_size, U rhs_size, U middle)
		{
			U p0 = 0;
			U p1 = lhs_size;
			rhs_size += lhs_size;
			while (p1 < rhs_size)
			{
				if (p0 == lhs_size || ptr[p0] > ptr[p1[)
				{
					swap(ptr[middle], ptr[p1]);
					++p1;
				}
				else
				{
					swap(ptr[middle], ptr[p0]);
					++p0;
				}
				++middle;
			}

			if (middle != p0)
			{
				block_swap(ptr + middle, ptr + p0, lhs_size - p0);
			}
		}

		template <typename T, typename U = size_t>
		constexpr void merge_right(T* ptr, U lhs_size, U rhs_size, U middle)
		{
			U p1 = lhs_size - 1;
			U p2 = p1 + rhs_size;
			U p0 = p2 + middle;

			while (true)
			{
				if (p0 == lhs_size || ptr[p0] > ptr[p1[)
				{
					swap(ptr[p0], ptr[p1]);
					--p1;
				}
				else
				{
					swap(ptr[p0], ptr[p2]);
					--p2;
				}
				--p0;
			}

			if (p2 == p0)
				return;

			while (p2 >= lhs_size)
			{
				swap(ptr[p0], ptr[p2]);
				--p0;
				--p2;
			}
		}

		template <typename T, typename U = size_t>
		constexpr void build_blocks(T* array, U size, T* external_buffer, U external_buffer_size)
		{
		}

		template <typename T, typename U = size_t>
		constexpr void lazy_merge_sort(T* array, U size)
		{
			T tmp;
			for (U i = 1; i < size; i += 2)
				conditional_swap(array[i] < array[i - 1], array[i], array[i - 1]);

			U merge_size = 2;
			while (merge_size < size)
			{
				const U next = merge_size * 2;
				U i = 0;
				for (; i < size - next; i += next)
					merge_bufferless(array + i, merge_size, merge_size);
				const U rest = size - i;
				if (rest > merge_size)
					merge_bufferless(array + i, merge_size, rest - merge_size);
				merge_size = next;
			}
		}

		template <typename T, typename U = size_t, typename S = ptrdiff_t>
		constexpr void insertion_sort(T* array, U size)
		{
			T tmp;
			for (U i = 1; i < size; ++i)
			{
				S j = i - 1;
				move(tmp, array[i]);
				if (array[i] >= array[0])
				{
					for (; array[j] > tmp; --j)
						move(array[j + 1], array[j]);
					move(array[j + 1], tmp);
				}
				else
				{
					for (; j != 0; --j)
						move(array[j + 1], array[j]);
					move(array[0], tmp);
				}
			}
		}

		template <typename T, typename U = size_t>
		constexpr void grail_sort_entry_point(T* array, U size, T* external_buffer, U external_buffer_size)
		{
			U block_size = 16;
			while (block_size * block_size < size)
				block_size *= 2;

			U key_buffer_size = ((size - 1) / block_size) + 1;
			const U desired_key_count = key_buffer_size + block_size;
			const U key_count = gather_keys(array, size, desired_key_count);

			bool suboptimal = false;
			if (key_count < desired_key_count)
			{
				if (key_count < 4)
					return lazy_merge_sort(array, size);

				key_buffer_size = block_size;
				block_size = 0;
				suboptimal = true;

				while (key_buffer_size > key_count)
					key_buffer_size /= 2;
			}

			const U key_buffer_end = block_size + key_buffer_size;
			U buffer_size = block_size;
			if (suboptimal)
				buffer_size = key_buffer_size;

			build_blocks(array, key_buffer_end, size - key_buffer_end, key_buffer_size, external_buffer, external_buffer_size);

			while (true)
			{
				const auto next = buffer_size * 2;
				if (size - key_buffer_end > buffer_size)
					break;
				buffer_size = next;
			}

			insertion_sort(array, key_buffer_end);
			lazy_merge_sort(array, size, size - key_buffer_end);
			return 0;
		}
	}

	constexpr size_t STATIC_BUFFER_SIZE = 512;

	enum class buffer_type
	{
		STATIC,
		NONE,
		DYNAMIC,
	};

	using allocate_callback = void*(*)(size_t size);
	using deallocate_callback = void* (*)(void* buffer, size_t size);

	struct dynamic_buffer_options
	{
		size_t buffer_size;
		allocate_callback allocate;
		deallocate_callback deallocate;
	};

	struct sort_options
	{
		buffer_type buffer_type;
		const dynamic_buffer_options* dynamic_buffer_options;
	};

	template <typename T, typename U = size_t>
	constexpr void sort(T* array, U size, sort_options options = {})
	{
		if (size < 16)
			return detail::insertion_sort(array, size);

		switch (options.buffer_type)
		{
		case buffer_type::STATIC:
			{
				constexpr size_t K = STATIC_BUFFER_SIZE;
				T buffer[K];
				detail::grail_sort_entry_point(array, size, buffer, K);
			}
			break;
		case buffer_type::NONE:
			detail::grail_sort_entry_point(array, size, nullptr, 0);
			break;
		case buffer_type::DYNAMIC:
			{
				assert(options.dynamic_buffer_options != nullptr);
				
				const size_t buffer_size = options.dynamic_buffer_options->buffer_size;
				const size_t allocation_size = buffer_size * sizeof(T);

				if (options.dynamic_buffer_options->allocate != nullptr)
				{
					T* const buffer = options.dynamic_buffer_options->allocate(allocation_size);
					assert(buffer != nullptr);

					detail::grail_sort_entry_point(array, size, buffer, buffer_size);

					options.dynamic_buffer_options->deallocate(buffer, allocation_size);
				}
				else
				{
					T* const buffer = malloc(buffer_size);
					assert(buffer != nullptr);

					detail::grail_sort_entry_point(array, size, buffer, buffer_size);

					free(buffer);
				}
			}
			break;
		default:
			assert(false);
			break;
		}
	}
}