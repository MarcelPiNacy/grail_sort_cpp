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
#include <cstdint>
#include <utility>
#include <algorithm>



#if (defined(_DEBUG) || !defined(NDEBUG)) && !defined(GRAILSORT_DEBUG)
#define GRAILSORT_DEBUG
#endif



namespace grail_sort
{
	namespace config
	{
#ifdef GRAILSORT_DEBUG
#endif

#ifdef GRAILSORT_NO_EXCEPTIONS
		constexpr bool exceptions = false;
#else
		constexpr bool exceptions = true;
#endif
	}

	namespace detail
	{

		template <typename T>
		constexpr void move_construct(T& to, T& from) noexcept(config::exceptions)
		{
			new (&to) T(std::move(from));
		}

		template <typename Iterator, typename Int>
		constexpr void move_range(Iterator to, Iterator from, Int count) noexcept(config::exceptions)
		{
			for (Int i = 0; i < count; ++i)
				move_construct(to[i], from[i]);
		}

		template <typename T>
		constexpr int compare(const T& left, const T& right) noexcept(config::exceptions)
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

		template <typename Iterator, typename Int>
		constexpr void block_swap(Iterator left, Iterator right, Int size) noexcept(config::exceptions)
		{
			const auto left_end = left + size;
			while (left != left_end)
			{
				std::swap(*left, *right);
				++left;
				++right;
			}
		}

		template <typename Iterator, typename Int>
		constexpr void rotate(Iterator array, Int l1, Int l2) noexcept(config::exceptions)
		{
			while (l1 != 0 && l2 != 0)
			{
				if (l2 > l1)
				{
					block_swap<Iterator, Int>(array, array + l1, l1);
					array += l1;
					l2 -= l1;
				}
				else
				{
					block_swap<Iterator, Int>(array + l1 - l2, array + l1, l2);
					l1 -= l2;
				}
			}
		}

		template <typename Iterator, typename Int>
		constexpr Int lower_bound(Iterator array, Int size, Iterator key) noexcept(config::exceptions)
		{
			Int low = -1;
			Int high = size;
			while (low < high - 1)
			{
				const Int p = low + (high - low) / 2;
				if (array[p] >= *key)
					high = p;
				else
					low = p;
			}
			return high;
		}

		template <typename Iterator, typename Int>
		constexpr Int upper_bound(Iterator array, Int size, Iterator key) noexcept(config::exceptions)
		{
			Int low = -1;
			Int high = size;
			while (low < high - 1)
			{
				const Int p = low + (high - low) / 2;
				if (array[p] > *key)
					high = p;
				else
					low = p;
			}
			return high;
		}

		template <typename Iterator, typename Int>
		constexpr Int gather_keys(Iterator array, Int size, Int desired_key_count) noexcept(config::exceptions)
		{
			Int h0 = 0;
			Int h = 0;
			Int found_key_count = 0;
			for (Int i = 1; i < size && found_key_count < desired_key_count; ++i)
			{
				const Int p = lower_bound<Iterator, Int>(array + h0, h, array + i);
				if (p == h || array[i] != array[h0 + p])
				{
					rotate<Iterator, Int>(array + h0, h, i - (h0 + h));
					h0 = i - h;
					rotate<Iterator, Int>(array + h0 + p, h - p, 1);
					++h;
				}
			}
			rotate<Iterator, Int>(array, h0, h);
			return h;
		}

		template <typename Iterator, typename Int>
		constexpr void merge_left_inplace(Iterator array, Int l1, Int l2) noexcept(config::exceptions)
		{
			while (l1 != 0)
			{
				const Int p = lower_bound<Iterator, Int>(array + l1, l2, array);
				if (p != 0)
				{
					rotate<Iterator, Int>(array, l1, p);
					array += p;
					l2 -= p;
				}

				if (l2 == 0)
					break;

				do
				{
					++array;
					--l1;
				} while (l1 != 0 && *array <= array[l1]);
			}
		}

		template <typename Iterator, typename Int>
		constexpr void merge_right_inplace(Iterator array, Int l1, Int l2) noexcept(config::exceptions)
		{
			while (l2 != 0)
			{
				const Int p = upper_bound<Iterator, Int>(array, l1, array + (l1 + l2 - 1));
				if (p != l1)
				{
					rotate<Iterator, Int>(array + p, l1 - p, l2);
					l1 = p;
				}

				if (l1 == 0)
					break;

				do
				{
					--l2;
				} while (l2 != 0 && array[l1 - 1] <= array[l1 + l2 - 1]);
			}
		}

		template <typename Iterator, typename Int>
		constexpr void merge_inplace(Iterator array, Int l1, Int l2) noexcept(config::exceptions)
		{
			if (l1 < l2)
				merge_left_inplace<Iterator, Int>(array, l1, l2);
			else
				merge_right_inplace<Iterator, Int>(array, l1, l2);
		}

		template <typename Iterator, typename Int>
		constexpr void merge_left(Iterator array, Int l1, Int l2, Int m) noexcept(config::exceptions)
		{
			Int p0 = 0;
			Int p1 = l1;
			l2 += l1;
			
			while (p1 < l2)
			{
				if (p0 == l1 || array[p0] > array[p1])
				{
					std::swap(array[m], array[p1]);
					++m;
					++p1;
				}
				else
				{
					std::swap(array[m], array[p0]);
					++m;
					++p0;
				}
			}

			if (m != p0)
			{
				block_swap<Iterator, Int>(array + m, array + p0, l1 - p0);
			}
		}

		template <typename Iterator, typename Int>
		constexpr void merge_right(Iterator array, Int l1, Int l2, Int m) noexcept(config::exceptions)
		{
			Int p1 = l1 - 1;
			Int p2 = l2 + p1;
			Int p0 = p2 + m;

			while (p1 >= 0)
			{
				if (p2 < l1 || array[p1] > array[p2])
				{
					std::swap(array[p0], array[p1]);
					--p0;
					--p1;
				}
				else
				{
					std::swap(array[p0], array[p2]);
					--p0;
					--p2;
				}
			}

			if (p2 != p0)
			{
				while (p2 >= l1)
				{
					std::swap(array[p0], array[p2]);
					--p0;
					--p2;
				}
			}
		}

		template <typename Iterator, typename Int>
		constexpr void smart_merge(Iterator array, Int& ref_l1, Int& ref_type, Int l2, Int key_count) noexcept(config::exceptions)
		{
			Int p0 = -key_count;
			Int p1 = 0;
			Int p2 = ref_l1;
			Int q1 = p2;
			Int q2 = p2 + l2;
			Int type = 1 - ref_type;

			while (p1 < q1 && p2 < q2)
			{
				if (compare(array[p1], array[p2]) - type < 0)
				{
					std::swap(array[p0], array[p1]);
					++p0;
					++p1;
				}
				else
				{
					std::swap(array[p0], array[p2]);
					++p0;
					++p2;
				}
			}

			if (p1 < q1)
			{
				ref_l1 = q1 - p1;
				while (p1 < q1)
				{
					std::swap(array[q1], array[q2]);
					--q1;
					--q2;
				}
			}
			else
			{
				ref_l1 = q2 - p2;
				ref_type = type;
			}
		}

		template <typename Iterator, typename Int>
		constexpr void smart_merge_inplace(Iterator array, Int& ref_l1, Int& ref_type, Int l2) noexcept(config::exceptions)
		{
			if (l2 == 0)
				return;

			Int l1 = ref_l1;
			Int type = 1 - ref_type;
			if (l1 != 0 && (compare(array[l1 - 1], array[l1]) - type) >= 0)
			{
				while (l1 != 0)
				{
					const Int h = type ?
						lower_bound<Iterator, Int>(array+ l1, l2, array) :
						upper_bound<Iterator, Int>(array+ l1, l2, array);

					if (h != 0)
					{
						rotate<Iterator, Int>(array, l1, h);
						array += h;
						l2 -= h;
					}

					if (l2 == 0)
					{
						ref_l1 = l1;
						return;
					}

					do
					{
						++array;
						--l1;
					} while (l1 != 0 && compare(*array, array[l1]) - type < 0);
				}
			}
			ref_l1 = l2;
			ref_type = type;
		}

		template <typename Iterator, typename Int>
		constexpr void merge_left_using_external_buffer(Iterator array, Int l1, Int l2, Int m) noexcept(config::exceptions)
		{
			Int p0 = 0;
			Int p1 = l1;
			l2 += l1;

			while (p1 < l2)
			{
				if (p0 == l1 || array[p0] > array[p1])
				{
					move_construct(array[m], array[p1]);
					++m;
					++p1;
				}
				else
				{
					move_construct(array[m], array[p0]);
					++m;
					++p0;
				}

				if (m != p0)
				{
					while (p0 < l1)
					{
						move_construct(array[m], array[p0]);
						++m;
						++p0;
					}
				}
			}
		}

		template <typename Iterator, typename Int>
		constexpr void smart_merge_using_external_buffer(Iterator array, Int& ref_l1, Int& ref_type, Int l2, Int key_count) noexcept(config::exceptions)
		{
			Int p0 = -key_count;
			Int p1 = 0;
			Int p2 = ref_l1;
			Int q1 = p2;
			Int q2 = p2 + l2;
			Int type = 1 - ref_type;
			while (p1 < q1 && p2 < q2)
			{
				if (compare(array[p0], array[p1]) - type < 0)
				{
					move_construct(array[p0], array[p1]);
					++p0;
					++p1;
				}
				else
				{
					move_construct(array[p0], array[p2]);
					++p0;
					++p2;
				}
			}

			if (p1 < q1)
			{
				ref_l1 = q1 - p1;
				while (p1 < q1)
				{
					move_construct(array[q2], array[q1]);
					--q2;
					--q1;
				}
			}
			else
			{
				ref_l1 = q2 - p2;
				ref_type = type;
			}
		}

		template <typename Iterator, typename Int>
		constexpr void merge_buffers_left_using_external_buffer(Iterator array, Iterator keys, Iterator median, Int block_count, Int block_size, Int block_count_2, Int last) noexcept(config::exceptions)
		{
			if (block_count == 0)
			{
				merge_left_using_external_buffer<Iterator, Int>(array, block_count_2 * block_size, last, -block_size);
				return;
			}

			Int lrest = block_size;
			Int frest = (Int)!(*keys < *median);
			Int p = block_size;

			for (Int c = 1; c < block_count; ++c)
			{
				Int prest = p - lrest;
				Int fnext = (Int)!(keys[c] < *median);
				if (fnext == frest)
				{
					move_range<Iterator, Int>(array + prest - block_size, array + prest, lrest);
					prest = p;
					lrest = block_size;
				}
				else
				{
					smart_merge_using_external_buffer<Iterator, Int>(array + prest, lrest, frest, block_size, block_size);
				}

				p += block_size;
			}

			Int prest = p;
			if (last != 0)
			{
				Int plast = p + block_size * block_count_2;
				if (frest != 0)
				{
					move_range<Iterator, Int>(array + prest - block_size, array + prest, lrest);
					prest = p;
					lrest = block_size * block_count_2;
					frest = 0;
				}
				else
				{
					lrest += block_size * block_count_2;
				}
				merge_left_using_external_buffer<Iterator, Int>(array + prest, lrest, last, -block_size);
			}
			else
			{
				move_range<Iterator, Int>(array + prest - block_size, array + prest, lrest);
			}
		}

		template <typename Iterator, typename Int>
		constexpr void build_blocks(Iterator array, Int l, Int k, Iterator external_buffer, Int external_buffer_size) noexcept(config::exceptions)
		{
			Int kb = k < external_buffer_size ? k : external_buffer_size;

			while ((kb & (kb - 1)) != 0)
				kb &= (kb - 1);

			Int h = 2;
			if (kb != 0)
			{
				move_range<Iterator, Int>(external_buffer, array - kb, kb);

				for (Int m = 1; m < l; m += 2)
				{
					Int u = (Int)(array[m - 1] > array[m]);
					move_construct(array[m - 3], array[m - 1 + u]);
					move_construct(array[m - 2], array[m - u]);
				}

				if ((l & 1) != 0)
					move_construct(array[l - 3], array[l - 1]);

				array -= 2;

				while (h < kb)
				{
					const Int nh = h * 2;
					Int p0 = 0;
					Int p1 = l - nh;
					
					while (p0 <= p1)
					{
						merge_left_using_external_buffer<Iterator, Int>(array + p0, h, h, -h);
						p0 += nh;
					}
					
					Int rest = l - p0;
					if (rest > h)
					{
						merge_left_using_external_buffer<Iterator, Int>(array + p0, h, rest - h, -h);
					}
					else
					{
						while (p0 < l)
						{
							move_construct(array[p0 - h], array[p0]);
							++p0;
						}
					}
					array -= h;
					h = nh;
				}
				move_range<Iterator, Int>(array + l, external_buffer, kb);
			}
			else
			{
				for (Int m = 1; m < l; m += 2)
				{
					Int u = (Int)(array[m - 1] > array[m]);
					std::swap(array[m - 3], array[m - 1 + u]);
					std::swap(array[m - 2], array[m - u]);
				}

				if ((l & 1) != 0)
					std::swap(array[l - 1], array[l - 3]);

				array -= 2;
			}

			while (h < k)
			{
				const Int nh = h * 2;
				Int p0 = 0;
				Int p1=l - nh;
				
				while (p0 <= p1)
				{
					merge_left<Iterator, Int>(array + p0, h, h, -h);
					p0 += nh;
				}

				const Int rest = l - p0;
				if (rest > h)
					merge_left<Iterator, Int>(array + p0, h, rest - h, -h);
				else
					rotate<Iterator, Int>(array + p0 - h, h, rest);
				array -= h;
				h = nh;
			}

			const Int k2 = k * 2;

			Int rest = l % k2;
			Int p = l - rest;
			if (rest <= k)
				rotate<Iterator, Int>(array + p, rest, k);
			else
				merge_right<Iterator, Int>(array + p, k, rest - k, k);


			while (p != 0)
			{
				p -= k2;
				merge_right<Iterator, Int>(array + p, k, k, k);
			}
		}

		template <typename Iterator, typename Int>
		constexpr void merge_buffers_left(Iterator array, Iterator keys, Iterator median, Int block_count, Int block_size, bool has_buffer, Int block_count_2, Int last) noexcept(config::exceptions)
		{
			if (block_count == 0)
			{
				const Int l = block_count_2 * block_size;
				if (has_buffer)
					merge_left<Iterator, Int>(array, l, last, -block_size);
				else
					merge_inplace<Iterator, Int>(array, l, last);
				return;
			}

			Int lrest = block_size;
			Int frest = (Int)!(*keys < *median);
			Int prest = 0;
			Int p = block_size;

			for (Int c = 1; c < block_count; ++c)
			{
				Int prest = p - lrest;
				Int fnext = (Int)!(keys[c] < *median);
				if (fnext == frest)
				{
					if (has_buffer)
						block_swap<Iterator, Int>(array + prest - block_size, array + prest, lrest);
					prest = p;
					lrest = block_size;
				}
				else
				{
					if (has_buffer)
						smart_merge<Iterator, Int>(array + prest, lrest, frest, block_size, block_size);
					else
						smart_merge_inplace<Iterator, Int>(array + prest, lrest, frest, block_size);
				}
				p += block_size;
			}

			const Int bk2 = block_size * block_count_2;

			prest = p - lrest;
			if (last != 0)
			{
				Int plast = p + bk2;
				if (frest != 0)
				{
					if (has_buffer)
						block_swap<Iterator, Int>(array + prest - block_size, array + prest, lrest);
					prest = p;
					lrest = bk2;
					frest = 0;
				}
				else
				{
					lrest += bk2;
				}

				if (has_buffer)
					merge_left<Iterator, Int>(array + prest, lrest, last, -block_size);
				else
					merge_inplace<Iterator, Int>(array + prest, lrest, last);
			}
			else
			{
				if (has_buffer)
					block_swap<Iterator, Int>(array + prest, array + prest - block_size, lrest);
			}
		}

		template <typename Iterator, typename Int>
		constexpr void insertion_sort_classic(Iterator array, Int size) noexcept(config::exceptions)
		{
			for (Int i = 1; i < size; ++i)
			{
				Int j = i - 1;
				auto tmp = std::move(array[i]);
				for (; j >= 0 && array[j] > tmp; --j)
					move_construct(array[j + 1], array[j]);
				move_construct(array[j + 1], tmp);
			}
		}

		template <typename Iterator, typename Int>
		constexpr void insertion_sort_stable(Iterator array, Int size) noexcept(config::exceptions)
		{
			if (size < 8)
			{
				insertion_sort_classic(array, size);
				return;
			}

			// Find min element, move it to the front, run unguarded insertion sort.
			Int min = 0;
			for (Int i = 1; i < size; ++i)
				if (array[i] < array[min])
					min = i;
			auto tmp = std::move(array[min]);
			for (Int i = min - 1; i >= 0; ++i)
				move_construct(array[i + 1], array[i]);
			move_construct(array[0], tmp);
			for (Int i = 1; i < size; ++i)
			{
				Int j = i - 1;
				auto tmp = std::move(array[i]);
				for (; array[j] > tmp; --j)
					move_construct(array[j + 1], array[j]);
				move_construct(array[j + 1], tmp);
			}
		}

		template <typename Iterator, typename Int>
		constexpr void insertion_sort_unstable(Iterator array, Int size) noexcept(config::exceptions)
		{
			for (Int i = 1; i < size; ++i)
			{
				if (array[i] < array[0])
					std::swap(array[0], array[i]);

				auto tmp = std::move(array[i]);
				Int j = i - 1;
				for (; array[j] > tmp; --j)
					move_construct(array[j + 1], array[j]);
				move_construct(array[j + 1], tmp);
			}
		}

		template <typename Iterator, typename Int>
		constexpr void lazy_merge_sort(Iterator array, Int size) noexcept(config::exceptions)
		{
			for (Int m = 1; m < size; m += 2)
				if (array[m - 1] > array[m])
					std::swap(array[m - 1], array[m]);

			for (Int m = 2; m < size;)
			{
				const Int nm = m * 2;
				Int p0 = 0;
				for (Int p1 = size - nm; p0 <= p1; p0 += nm)
					merge_inplace<Iterator, Int>(array + p0, m, m);
				const Int rest = size - p0;
				if (rest > m)
					merge_inplace<Iterator, Int>(array + p0, m, rest - m);
				m = nm;
			}
		}

		template <typename Iterator, typename Int>
		constexpr void combine_blocks(Iterator array, Iterator keys, Int size, Int ll, Int block_size, bool has_buffer, Iterator external_buffer) noexcept(config::exceptions)
		{
			const Int ll2 = ll * 2;
			Int m = size / ll2;
			Int lrest = size % ll2;
			Int key_count = ll2 / block_size;
			
			if (lrest <= ll)
			{
				size -= lrest;
				lrest = 0;
			}
			
			if (external_buffer != Iterator())
				move_range<Iterator, Int>(external_buffer, array - block_size, block_size);

			for (Int b = 0; b <= m; ++b)
			{
				if (b == m && lrest == 0)
					break;
				Iterator array1 = array + b * ll2;
				Int bk = ll2;
				if (b == m)
					bk = lrest;
				bk /= block_size;
				insertion_sort_stable<Iterator, Int>(keys, bk + (Int)(b == m));
				Int median = ll / block_size;
				for (Int u = 1; u < bk; ++u)
				{
					Int p0 = u - 1;
					Int p = p0;
					for (Int v = u; v < bk; ++v)
					{
						const int cmp = compare(array1[p * block_size], array1[v * block_size]);
						if (cmp > 0 || (cmp == 0 && keys[p] > keys[v]))
							p = v;
					}

					if (p != p0)
					{
						block_swap<Iterator, Int>(array1 + p0 * block_size, array1 + p * block_size, block_size);
						std::swap(keys[p0], keys[p]);
						if (median == p0 || median == p)
							median ^= (p0 ^ p);
					}
				}

				Int bk2 = 0;
				Int llast = 0;

				if (b == m)
					llast = lrest % block_size;

				if (llast != 0)
				{
					while (bk2 < bk && array1[bk * block_size] < array1[bk - bk2 - 1])
						++bk2;
				}

				if (external_buffer != Iterator()) //is null?
				{
					merge_buffers_left_using_external_buffer<Iterator, Int>(array1, keys, keys + median, bk - bk2, block_size, bk2, llast);
				}
				else
				{
					merge_buffers_left<Iterator, Int>(array1, keys, keys + median, bk - bk2, block_size, has_buffer, bk2, llast);
				}
			}

			if (external_buffer != Iterator())
			{
				while (true)
				{
					--size;
					if (size < 0)
						break;
					move_construct(array[size], array[size - block_size]);
				}	
				move_range(array - block_size, external_buffer, block_size);
			}
			else if (has_buffer)
			{
				while (true)
				{
					--size;
					if (size < 0)
						break;
					std::swap(array[size], array[size - block_size]);
				}
			}
		}

		template <typename Iterator, typename Int>
		constexpr void entry_point(Iterator array, Int size, Iterator external_buffer, Int external_buffer_size) noexcept(config::exceptions)
		{
			if (size < 16)
			{
				insertion_sort_classic<Iterator, Int>(array, size);
				return;
			}

			Int block_size = 1;
			while (block_size * block_size < size)
				block_size *= 2;
			
			Int key_count = 1 + (size - 1) / block_size;
			const Int desired_key_count = key_count + block_size;
			Int found_key_count = gather_keys<Iterator, Int>(array, size, desired_key_count);
			bool has_buffer = true;

			if (found_key_count < desired_key_count)
			{
				if (key_count < 4)
				{
					lazy_merge_sort<Iterator, Int>(array, size);
					return;
				}

				key_count = block_size;
				while (key_count > found_key_count)
					key_count /= 2;
				has_buffer = false;
				block_size = 0;
			}

			Int offset = block_size + key_count;
			Int cb = key_count;
			Int delta = size - offset;

			if (has_buffer)
			{
				build_blocks<Iterator, Int>(array + offset, delta, cb, external_buffer, external_buffer_size);
			}
			else
			{
				cb = block_size;
				build_blocks<Iterator, Int>(array + offset, delta, cb, Iterator(), 0);
			}

			cb *= 2;
			while (delta > cb)
			{
				Int b2 = block_size;
				bool hb2 = has_buffer;
				if (!hb2)
				{
					if (key_count > 4 && key_count / 8 * key_count >= cb)
					{
						b2 = key_count / 2;
						hb2 = true;
					}
					else
					{
						Int nk = 1;
						intmax_t s = (intmax_t)cb * key_count / 2;
						while (nk < key_count && s != 0)
						{
							nk *= 2;
							s /= 8;
						}
						b2 = (2 * cb) / nk;
					}
				}
				else
				{
					if (external_buffer_size != 0)
					{
						while (b2 > external_buffer_size && b2 * b2 > 2 * cb)
							b2 /= 2;
					}
				}

				const bool flag = hb2 && b2 <= external_buffer_size;
				combine_blocks<Iterator, Int>(array, array + offset, delta, cb, b2, hb2, flag ? external_buffer : Iterator());
				cb *= 2;
			}

			insertion_sort_unstable<Iterator, Int>(array, offset);
			merge_inplace<Iterator, Int>(array, offset, delta);
		}
	}

	struct alignas(sizeof(size_t) * 2) sort_options
	{
		/// <summary>
		/// Untyped pointer to an external buffer.
		/// </summary>
		void*	buffer_ptr;

		/// <summary>
		/// The size of the buffer, in elements.
		/// </summary>
		size_t	buffer_size;
	};

	template <typename T, typename Int = ptrdiff_t>
	constexpr void sort(T* array, Int size, sort_options options = {}) noexcept(grail_sort::config::exceptions)
	{
		detail::entry_point<T*, Int>(array, size, (T*)options.buffer_ptr, options.buffer_size);
	}
}