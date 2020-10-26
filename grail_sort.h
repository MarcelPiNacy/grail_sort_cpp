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

/*
	https://github.com/Mrrl/GrailSort/blob/master/GrailSort.h
*/

#pragma once
#include <cstdint>
#include <utility>
#include <algorithm>



#ifdef _MSVC_LANG

#define GRAILSORT_ASSUME(expression) __assume((expression))

#else

#define GRAILSORT_ASSUME(expression)

#endif



#if (defined(_DEBUG) || !defined(NDEBUG)) && !defined(GRAILSORT_DEBUG)
#include <cassert>

#define GRAILSORT_DEBUG
#define GRAILSORT_ASSERT(expression) assert(expression)
#define GRAILSORT_ASSERT_POW2(expression) GRAILSORT_ASSERT(((expression) & ((expression) - 1)) == 0)

#else

#define GRAILSORT_ASSERT(expression)

#endif



#ifdef GRAILSORT_NO_EXCEPTIONS
#define GRAILSORT_NOTHROW noexcept
#else
#define GRAILSORT_NOTHROW
#endif



namespace grail_sort
{
	namespace detail
	{

		/*
			Type info:
				- Iterator: A random access iterator type.
				- Int: A signed integer type.
		*/

		template <typename T>
		constexpr int compare(const T& left, const T& right) GRAILSORT_NOTHROW
		{
			if constexpr (std::is_arithmetic_v<T>) //TODO: Handle underflow.
			{
				return left - right;
			}
			else
			{
				if (left == right)
					return 0;
				int result = 1;
				if (left < right)
					result = -1;
				return result;
			}
		}

		template <typename T>
		constexpr void move_construct(T& to, T& from) GRAILSORT_NOTHROW
		{
			new (&to) T(std::move(from));
		}

		template <typename T>
		constexpr void swap(T& left, T& right) GRAILSORT_NOTHROW
		{
			std::swap(left, right);
		}

		template <typename Iterator, typename Int>
		constexpr void block_move(Iterator to, Iterator from, Int count) GRAILSORT_NOTHROW
		{
			using T = std::remove_reference_t<decltype(*to)>;
			if constexpr (std::is_pointer_v<Iterator> && std::is_trivially_move_constructible_v<T>)
			{
				memcpy(to, from, count * sizeof(T));
			}
			else
			{
				std::move(from, from + count, to);
			}
		}

		template <typename Iterator, typename Int>
		constexpr void block_swap(Iterator left, Iterator right, Int size) GRAILSORT_NOTHROW
		{
			const auto left_end = left + size;
			while (left != left_end)
			{
				swap(*left, *right);
				++left;
				++right;
			}
		}

		template <typename Iterator, typename Int>
		constexpr void rotate(Iterator array, Int l1, Int l2) GRAILSORT_NOTHROW
		{
			while (l1 != 0 && l2 != 0)
			{
				if (l1 <= l2)
				{
					block_swap(array, array + l1, l1);
					array += l1;
					l2 -= l1;
				}
				else
				{
					block_swap(array + (l1 - l2), array + l1, l2);
					l1 -= l2;
				}
			}
		}

		template <typename Iterator, typename Int>
		constexpr Int lower_bound(Iterator array, Int size, Iterator key) GRAILSORT_NOTHROW
		{
			Int low = -1;
			Int high = size;
			while (low < high - 1)
			{
				const Int p = low + (high - low) / 2;
				const bool flag = *(array + p) >= *key;
				if (flag)
					high = p;
				else
					low = p;
			}
			return high;
		}

		template <typename Iterator, typename Int>
		constexpr Int upper_bound(Iterator array, Int size, Iterator key) GRAILSORT_NOTHROW
		{
			Int low = -1;
			Int high = size;
			while (low < high - 1)
			{
				const Int p = low + (high - low) / 2;
				const bool flag = *(array + p) > *key;
				if (flag)
					high = p;
				else
					low = p;
			}
			return high;
		}

		template <typename Iterator, typename Int>
		constexpr Int gather_keys(Iterator array, Int size, Int desired_key_count) GRAILSORT_NOTHROW
		{
			Int h0 = 0;
			Int h = 1;
			for (Int i = 1; i < size && h < desired_key_count; ++i)
			{
				const Int p = lower_bound(array + h0, h, array + i);
				if (p == h || *(array + i) != *(array + (h0 + p)))
				{
					rotate(array + h0, h, i - (h0 + h));
					h0 = i - h;
					rotate(array + h0 + p, h - p, 1);
					++h;
				}
			}
			rotate(array, h0, h);
			return h;
		}

		template <typename Iterator, typename Int>
		constexpr void merge_left_inplace(Iterator array, Int left_size, Int right_size) GRAILSORT_NOTHROW
		{
			while (left_size != 0)
			{
				const Int p = lower_bound(array + left_size, right_size, array);
				if (p != 0)
				{
					rotate(array, left_size, p);
					array += p;
					right_size -= p;
				}

				if (right_size == 0)
				{
					break;
				}

				do
				{
					++array;
					--left_size;
				} while (left_size != 0 && *array <= *(array + left_size));
			}
		}

		template <typename Iterator, typename Int>
		constexpr void merge_right_inplace(Iterator array, Int left_size, Int right_size) GRAILSORT_NOTHROW
		{
			while (right_size != 0)
			{
				const Int p = upper_bound(array, left_size, array + (left_size + right_size - 1));
				if (p != left_size)
				{
					rotate(array + p, left_size - p, right_size);
					left_size = p;
				}

				if (left_size == 0)
				{
					break;
				}

				do
				{
					--right_size;
				} while (right_size != 0 && *(array + (left_size - 1)) <= *(array + (left_size + right_size - 1)));
			}
		}

		template <typename Iterator, typename Int>
		constexpr void merge_inplace(Iterator array, Int left_size, Int right_size) GRAILSORT_NOTHROW
		{
			if (left_size < right_size)
			{
				merge_left_inplace(array, left_size, right_size);
			}
			else
			{
				merge_right_inplace(array, left_size, right_size);
			}
		}

		template <typename Iterator, typename Int>
		constexpr void merge_left(Iterator array, Int left_size, Int right_size, Int m) GRAILSORT_NOTHROW
		{
			Int p0 = 0;
			Int p1 = left_size;
			right_size += left_size;
			
			while (p1 < right_size)
			{
				if (p0 == left_size || *(array + p0) > * (array + p1))
				{
					++m;
					++p1;
					swap(*(array + m), *(array + p1));
				}
				else
				{
					++m;
					++p0;
					swap(*(array + m), *(array + p0));
				}
			}

			if (m != p0)
			{
				block_swap(array + m, array + p0, left_size - p0);
			}
		}

		template <typename Iterator, typename Int>
		constexpr void merge_right(Iterator array, Int left_size, Int right_size, Int m) GRAILSORT_NOTHROW
		{
			Int p1 = left_size - 1;
			Int p2 = right_size + p1;
			Int p0 = p2 + m;

			while (p1 >= 0)
			{
				if (p2 < left_size || *(array + p1) > * (array + p2))
				{
					--p0;
					--p1;
					swap(*(array + p0), *(array + p1));
				}
				else
				{
					--p0;
					--p2;
					swap(*(array + p0), *(array + p2));
				}
			}

			if (p2 != p0)
			{
				while (p2 >= left_size)
				{
					--p0;
					--p2;
					swap(*(array + p0), *(array + p2));
				}
			}
		}

		template <typename Iterator, typename Int>
		constexpr void smart_merge(Iterator array, Int& ref_left_size, Int& ref_type, Int right_size, Int key_count) GRAILSORT_NOTHROW
		{
			Int p0 = -key_count;
			Int p1 = 0;
			Int p2 = ref_left_size;
			Int q1 = p2;
			Int q2 = p2 + right_size;
			Int type = 1 - ref_type;

			while (p1 < q1 && p2 < q2)
			{
				if (compare(*(array + p1), *(array + p2)) - type < 0)
				{
					++p0;
					++p1;
					swap(*(array + p0), *(array + p1));
				}
				else
				{
					++p0;
					++p2;
					swap(*(array + p0), *(array + p2));
				}
			}

			if (p1 < q1)
			{
				ref_left_size = q1 - p1;
				while (p1 < q1)
				{
					--q1;
					--q2;
					swap(*(array + q1), *(array + q2));
				}
			}
			else
			{
				ref_left_size = q2 - p2;
				ref_type = type;
			}
		}

		template <typename Iterator, typename Int>
		constexpr void smart_merge_inplace(Iterator array, Int& ref_left_size, Int& ref_type, Int right_size) GRAILSORT_NOTHROW
		{
			if (right_size == 0)
				return;

			Int l1 = ref_left_size;
			Int type = 1 - ref_type;
			if (l1 != 0 && (compare(*(array + (l1 - 1)), *(array + l1)) - type) >= 0)
			{
				while (l1 != 0)
				{
					const Int h = type ?
						lower_bound(array+ l1, right_size, array) :
						upper_bound(array+ l1, right_size, array);

					if (h != 0)
					{
						rotate(array, l1, h);
						array += h;
						right_size -= h;
					}

					if (right_size == 0)
					{
						ref_left_size = l1;
						return;
					}

					do
					{
						++array;
						--l1;
					} while (l1 != 0 && compare(*array, *(array + l1)) - type < 0);
				}
			}
			ref_left_size = right_size;
			ref_type = type;
		}

		template <typename Iterator, typename Int>
		constexpr void merge_left_using_external_buffer(Iterator array, Int left_size, Int right_size, Int m) GRAILSORT_NOTHROW
		{
			Int p0 = 0;
			Int p1 = left_size;
			right_size += left_size;

			while (p1 < right_size)
			{
				if (p0 == left_size || *(array + p0) > * (array + p1))
				{
					move_construct(*(array + m), *(array + p1));
					++m;
					++p1;
				}
				else
				{
					move_construct(*(array + m), *(array + p0));
					++m;
					++p0;
				}

				if (m != p0)
				{
					while (p0 < left_size)
					{
						move_construct(*(array + m), *(array + p0));
						++m;
						++p0;
					}
				}
			}
		}

		template <typename Iterator, typename Int>
		constexpr void smart_merge_using_external_buffer(Iterator array, Int& ref_left_size, Int& ref_type, Int right_size, Int key_count) GRAILSORT_NOTHROW
		{
			Int p0 = -key_count;
			Int p1 = 0;
			Int p2 = ref_left_size;
			Int q1 = p2;
			Int q2 = p2 + right_size;
			Int type = 1 - ref_type;
			while (p1 < q1 && p2 < q2)
			{
				if (compare(*(array + p0), *(array + p1)) - type < 0)
				{
					move_construct(*(array + p0), *(array + p1));
					++p0;
					++p1;
				}
				else
				{
					move_construct(*(array + p0), *(array + p2));
					++p0;
					++p2;
				}
			}

			if (p1 < q1)
			{
				ref_left_size = q1 - p1;
				while (p1 < q1)
				{
					move_construct(*(array + q2), *(array + q1));
					--q2;
					--q1;
				}
			}
			else
			{
				ref_left_size = q2 - p2;
				ref_type = type;
			}
		}

		template <typename Iterator, typename Int>
		constexpr void merge_buffers_left_using_external_buffer(Iterator array, Iterator keys, Iterator median, Int block_count, Int block_size, Int block_count_2, Int last) GRAILSORT_NOTHROW
		{
			if (block_count == 0)
			{
				merge_left_using_external_buffer(array, block_count_2 * block_size, last, -block_size);
				return;
			}

			Int lrest = block_size;
			Int frest = (Int)!(*keys < *median);
			Int p = block_size;

			for (Int i = 1; i < block_count; ++i)
			{
				Int prest = p - lrest;
				Int fnext = (Int)!(*(keys + i) < *median);
				if (fnext == frest)
				{
					block_move(array + prest - block_size, array + prest, lrest);
					prest = p;
					lrest = block_size;
				}
				else
				{
					smart_merge_using_external_buffer(array + prest, lrest, frest, block_size, block_size);
				}

				p += block_size;
			}

			Int prest = p;
			if (last != 0)
			{
				Int plast = p + block_size * block_count_2;
				if (frest != 0)
				{
					block_move(array + prest - block_size, array + prest, lrest);
					prest = p;
					lrest = block_size * block_count_2;
					frest = 0;
				}
				else
				{
					lrest += block_size * block_count_2;
				}

				merge_left_using_external_buffer(array + prest, lrest, last, -block_size);
			}
			else
			{
				block_move(array + prest - block_size, array + prest, lrest);
			}
		}

		template <typename Iterator, typename Int>
		constexpr void merge_buffers_left(Iterator array, Iterator keys, Iterator median, Int block_count, Int block_size, bool has_buffer, Int block_count_2, Int last) GRAILSORT_NOTHROW
		{
			if (block_count == 0)
			{
				const Int l = block_count_2 * block_size;
				if (has_buffer)
				{
					merge_left(array, l, last, -block_size);
				}
				else
				{
					merge_inplace(array, l, last);
				}
				return;
			}

			Int lrest = block_size;
			Int frest = (Int)!(*keys < *median);
			Int prest = 0;
			Int p = block_size;

			for (Int i = 1; i < block_count; ++i)
			{
				Int prest = p - lrest;
				Int fnext = (Int)!(*(keys + i) < *median);
				if (fnext == frest)
				{
					if (has_buffer)
					{
						block_swap(array + prest - block_size, array + prest, lrest);
					}
					prest = p;
					lrest = block_size;
				}
				else
				{
					if (has_buffer)
					{
						smart_merge(array + prest, lrest, frest, block_size, block_size);
					}
					else
					{
						smart_merge_inplace(array + prest, lrest, frest, block_size);
					}
				}
				p += block_size;
			}

			prest = p - lrest;
			if (last != 0)
			{
				const Int bk2 = block_size * block_count_2;
				Int plast = p + bk2;
				if (frest != 0)
				{
					if (has_buffer)
					{
						block_swap(array + prest - block_size, array + prest, lrest);
					}

					prest = p;
					lrest = bk2;
					frest = 0;
				}
				else
				{
					lrest += bk2;
				}

				if (has_buffer)
				{
					merge_left(array + prest, lrest, last, -block_size);
				}
				else
				{
					merge_inplace(array + prest, lrest, last);
				}
			}
			else
			{
				if (has_buffer)
				{
					block_swap(array + prest, array + (prest - block_size), lrest);
				}
			}
		}

		template <typename Iterator, typename Int>
		constexpr void build_blocks(Iterator array, Int size, Int internal_buffer_size, Iterator external_buffer, Int external_buffer_size) GRAILSORT_NOTHROW
		{
			Int buffer_size = external_buffer_size;
			if (internal_buffer_size < external_buffer_size)
				buffer_size = internal_buffer_size;

			while (true)
			{
				const Int mask = buffer_size - 1;
				const Int next = buffer_size & mask;
				if (next == 0)
					break;
				buffer_size = next;
			}

			Int i = 2;
			if (buffer_size != 0)
			{
				block_move(external_buffer, array - buffer_size, buffer_size);

				for (Int j = 1; j < size; j += 2)
				{
					const bool flag = *(array + (j - 1)) > *(array + j);
					move_construct(*(array + (j - 3)), *(array + (j - 1 + (Int)flag)));
					move_construct(*(array + (j - 2)), *(array + (j - (Int)flag)));
				}

				if ((size & 1) != 0)
				{
					move_construct(*(array + (size - 3)), *(array + (size - 1)));
				}

				array -= 2;

				while (i < buffer_size)
				{
					const Int next = i * 2;
					Int offset = 0;
					
					while (offset <= size - next)
					{
						merge_left_using_external_buffer(array + offset, i, i, -i);
						offset += next;
					}
					
					Int rest = size - offset;
					if (rest > i)
					{
						merge_left_using_external_buffer(array + offset, i, rest - i, -i);
					}
					else
					{
						while (offset < size)
						{
							move_construct(*(array + (offset - i)), *(array + offset));
							++offset;
						}
					}
					array -= i;
					i = next;
				}
				block_move(array + size, external_buffer, buffer_size);
			}
			else
			{
				for (Int j = 1; j < size; j += 2)
				{
					Int u = (Int)(*(array + (j - 1)) > *(array + j));
					swap(*(array + (j - 3)), *(array + (j - 1 + u)));
					swap(*(array + (j - 2)), *(array + (j - u)));
				}

				if ((size & 1) != 0)
				{
					swap(*(array + (size - 1)), *(array + (size - 3)));
				}

				array -= 2;
			}

			while (i < internal_buffer_size)
			{
				const Int next = i * 2;
				Int p0 = 0;
				
				while (p0 <= size - next)
				{
					merge_left(array + p0, i, i, -i);
					p0 += next;
				}

				const Int rest = size - p0;
				if (rest > i)
				{
					merge_left(array + p0, i, rest - i, -i);
				}
				else
				{
					rotate(array + p0 - i, i, rest);
				}
				array -= i;
				i = next;
			}

			const Int k2 = internal_buffer_size * 2;

			Int rest = size % k2;
			Int p = size - rest;
			if (rest <= internal_buffer_size)
			{
				rotate(array + p, rest, internal_buffer_size);
			}
			else
			{
				merge_right(array + p, internal_buffer_size, rest - internal_buffer_size, internal_buffer_size);
			}


			while (p != 0)
			{
				p -= k2;
				merge_right(array + p, internal_buffer_size, internal_buffer_size, internal_buffer_size);
			}
		}

		template <typename Iterator, typename Int>
		constexpr void insertion_sort_classic(Iterator array, Int size) GRAILSORT_NOTHROW
		{
			GRAILSORT_ASSUME(size < 8);

			for (Int i = 1; i < size; ++i)
			{
				Int j = i - 1;
				auto tmp = std::move(*(array + i));
				for (; j >= 0 && *(array + j) > tmp; --j)
					move_construct(*(array + (j + 1)), *(array + j));
				move_construct(*(array + (j + 1)), tmp);
			}
		}

		template <typename Iterator, typename Int>
		constexpr void insertion_sort_stable(Iterator array, Int size) GRAILSORT_NOTHROW
		{
			if (size < 8)
				return insertion_sort_classic(array, size);
			Int min = 0;
			for (Int i = 1; i < size; ++i)
				if (*(array + i) < *(array + min))
					min = i;
			auto tmp = std::move(*(array + min));
			for (Int i = min; i > 0; --i)
				move_construct(*(array + i), *(array + i - 1));
			move_construct(*array, tmp);
			for (Int i = 1; i < size; ++i)
			{
				Int j = i - 1;
				auto tmp = std::move(*(array + i));
				for (; *(array + j) > tmp; --j)
					move_construct(*(array + j + 1), *(array + j));
				move_construct(*(array + j + 1), tmp);
			}
		}

		template <typename Iterator, typename Int>
		constexpr void insertion_sort_unstable(Iterator array, Int size) GRAILSORT_NOTHROW
		{
			for (Int i = 1; i < size; ++i)
			{
				if (*(array + i) < *array)
					swap(*array, *(array + i));
				auto tmp = std::move(*(array + i));
				Int j = i - 1;
				for (; *(array + j) > tmp; --j)
					move_construct(*(array + j + 1), *(array + j));
				move_construct(*(array + j + 1), tmp);
			}
		}

		template <typename Iterator, typename Int>
		constexpr void lazy_merge_sort(Iterator array, Int size) GRAILSORT_NOTHROW
		{
			for (Int i = 1; i < size; i += 2)
				if (*(array + i - 1) > *(array + i))
					swap(*(array + i - 1), *(array + i));

			for (Int i = 2; i < size;)
			{
				const Int nm = i * 2;
				Int p0 = 0;
				for (Int p1 = size - nm; p0 <= p1; p0 += nm)
					merge_inplace(array + p0, i, i);
				const Int rest = size - p0;
				if (rest > i)
					merge_inplace(array + p0, i, rest - i);
				i = nm;
			}
		}

		template <typename Iterator, typename Int>
		constexpr void combine_blocks(Iterator array, Iterator keys, Int size, Int ll, Int block_size, bool has_buffer, Iterator external_buffer) GRAILSORT_NOTHROW
		{
			const auto nil_iterator = Iterator();

			const Int ll2 = ll * 2;
			Int m = size / ll2;
			Int lrest = size % ll2;
			Int key_count = ll2 / block_size;
			
			if (lrest <= ll)
			{
				size -= lrest;
				lrest = 0;
			}
			
			if (external_buffer != nil_iterator)
			{
				block_move(external_buffer, array - block_size, block_size);
			}

			for (Int i = 0; i <= m; ++i)
			{
				const bool flag = i == m;
				if (flag && lrest == 0)
					break;
				
				const Int bk = (flag ? lrest : ll2) / block_size;
				
				insertion_sort_stable(keys, bk + (Int)flag);
				
				Int median = ll / block_size;
				const Iterator ptr = array + i * ll2;

				for (Int u = 1; u < bk; ++u)
				{
					const Int p0 = u - 1;
					Int p = p0;
					for (Int v = u; v < bk; ++v)
					{
						const auto cmp = compare(*(ptr + p * block_size), *(ptr + v * block_size));
						if (cmp > 0 || (cmp == 0 && *(keys + p) > * (keys + v)))
							p = v;
					}

					if (p != p0)
					{
						block_swap(ptr + p0 * block_size, ptr + p * block_size, block_size);
						swap(*(keys + p0), *(keys + p));
						if (median == p0 || median == p)
							median ^= p0 ^ p;
					}
				}

				Int bk2 = 0;
				const Int last = flag ? lrest % block_size : 0;

				if (last != 0)
				{
					while (bk2 < bk && *(ptr + bk * block_size) < *(ptr + (bk - bk2 - 1) * block_size))
						++bk2;
				}

				if (external_buffer != nil_iterator)
				{
					merge_buffers_left_using_external_buffer(ptr, keys, keys + median, bk - bk2, block_size, bk2, last);
				}
				else
				{
					merge_buffers_left(ptr, keys, keys + median, bk - bk2, block_size, has_buffer, bk2, last);
				}
			}

			--size;
			if (external_buffer != nil_iterator)
			{
				while (size >= 0)
				{
					move_construct(*(array + size), *(array + size - block_size));
					--size;
				}	

				block_move(array - block_size, external_buffer, block_size);
			}
			else if (has_buffer)
			{
				while (size >= 0)
				{
					swap(*(array + size), *(array + size - block_size));
					--size;
				}
			}
		}

		template <typename Iterator, typename Int>
		constexpr void entry_point(Iterator array, Int size, Iterator external_buffer, Int external_buffer_size) GRAILSORT_NOTHROW
		{
			if (size < 16)
			{
				insertion_sort_stable(array, size);
				return;
			}

			Int block_size = 4;
			while (block_size * block_size < size)
				block_size *= 2;
			
			Int key_count = 1 + (size - 1) / block_size;
			const Int desired_key_count = key_count + block_size;
			Int found_key_count = gather_keys(array, size, desired_key_count);
			const bool has_buffer = found_key_count >= desired_key_count;

			if (!has_buffer)
			{
				if (key_count < 4)
				{
					lazy_merge_sort(array, size);
					return;
				}

				key_count = block_size;
				while (key_count > found_key_count)
					key_count /= 2;
				block_size = 0;
			}

			const Int offset = block_size + key_count;
			Iterator begin = array + offset;
			Int range = size - offset;
			Int internal_buffer_size = key_count;

			if (has_buffer)
				internal_buffer_size = block_size;

			if (has_buffer)
			{
				build_blocks(begin, range, internal_buffer_size, external_buffer, external_buffer_size);
			}
			else
			{
				const auto nil_iterator = Iterator();
				build_blocks(begin, range, internal_buffer_size, nil_iterator, 0);
			}

			while (true)
			{
				internal_buffer_size *= 2;
				if (internal_buffer_size >= range)
					break;

				Int b2 = block_size;
				bool hb2 = has_buffer;
				if (!hb2)
				{
					if (key_count > 4 && key_count / 8 * key_count >= internal_buffer_size)
					{
						b2 = key_count / 2;
						hb2 = true;
					}
					else
					{
						Int nk = 1;
						intmax_t s = (intmax_t)internal_buffer_size * key_count / 2;
						while (nk < key_count && s != 0)
						{
							nk *= 2;
							s /= 8;
						}
						b2 = (2 * internal_buffer_size) / nk;
					}
				}
				else
				{
					if (external_buffer_size != 0)
					{
						while (b2 > external_buffer_size && b2 * b2 > 2 * internal_buffer_size)
							b2 /= 2;
					}
				}

				Iterator xb = {};
				if (hb2 && b2 <= external_buffer_size)
					xb = external_buffer;
				combine_blocks(begin, array, range, internal_buffer_size, b2, hb2, xb);
			}

			insertion_sort_unstable(array, offset); //All items are unique, stability isn't required.
			merge_inplace(array, offset, range);
		}
	}

	template <typename Iterator, typename Int = ptrdiff_t>
	constexpr void sort(Iterator begin, Iterator end) GRAILSORT_NOTHROW
	{
		detail::entry_point<Iterator, Int>(begin, std::distance(begin, end), Iterator(), 0);
	}
}