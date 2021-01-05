/*
	MIT License

	Copyright (c) 2013 Andrey Astrelin
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
#include <new>
#include <cassert>
#include <iterator>
#include <algorithm>
#include <type_traits>

#if !defined(GRAILSORT_DEBUG) && defined(_DEBUG) && !defined(NDEBUG)
#define GRAILSORT_DEBUG
#endif

#ifdef GRAILSORT_DEBUG
#include <intrin.h>
#define GRAILSORT_INVARIANT(...) assert(__VA_ARGS__)
#else
#define GRAILSORT_INVARIANT(...)
#endif



namespace grailsort
{
	namespace detail
	{
		template <typename J>
		struct grail_sort_external_buffer_helper_type
		{
			J external_buffer_begin;
			J external_buffer_end;

			constexpr void assign_external_buffer(J begin, J end)
			{
				external_buffer_begin = begin;
				external_buffer_end = end;
			}
		};

		template <>
		struct grail_sort_external_buffer_helper_type<void>
		{
		};

		template <typename I, typename J>
		struct grail_sort_helper : grail_sort_external_buffer_helper_type<J>
		{
			using iterator_traits = std::iterator_traits<I>;
			using value_type = typename iterator_traits::value_type;
			using difference_type = typename iterator_traits::difference_type;
			using size_type = std::make_unsigned_t<difference_type>;

			static constexpr size_type sqrt_of_unchecked(size_type size)
			{
				size_type r = 1;
				while (r * r < size)
					r *= 2;
				return r;
			}

			enum : size_type
			{
				CACHE_LINE_SIZE = std::hardware_constructive_interference_size,
				SMALL_SORT_THRESHOLD = std::max<size_type>(CACHE_LINE_SIZE / sizeof(value_type), 4),
				MEDIUM_SORT_THRESHOLD = 4 * std::min<size_type>(CACHE_LINE_SIZE / sizeof(value_type), 4),
				SMALL_SORT_THRESHOLD_SQRT = sqrt_of_unchecked(SMALL_SORT_THRESHOLD),
				LINEAR_SEARCH_THRESHOLD = SMALL_SORT_THRESHOLD,
				BASIC_INSERTION_SORT_THRESHOLD = 7,
				GATHER_UNIQUE_BASIC_THRESHOLD = CACHE_LINE_SIZE,
			};

			static constexpr size_type distance(I begin, I end)
			{
				GRAILSORT_INVARIANT(begin <= end);

				return (size_type)std::distance(begin, end);
			}

			static constexpr size_type sqrt_of(size_type size)
			{
				GRAILSORT_INVARIANT(size >= SMALL_SORT_THRESHOLD);

				size_type r = SMALL_SORT_THRESHOLD_SQRT;
				while (r * r < size)
					r *= 2;
				return r;
			}

			template <typename T>
			static constexpr void iter_move(I left, T&& right)
			{
				new (&*left) value_type(std::forward<T>(right));
			}

			static constexpr void iter_move(I left, I right)
			{
				iter_move(left, std::move(*right));
			}

			static constexpr void iter_swap(I left, I right)
			{
				std::iter_swap(left, right);
			}

			template <typename I2 = I>
			static constexpr void swap_range(I in_begin, I in_end, I2 out_begin)
			{
				GRAILSORT_INVARIANT(in_begin <= in_end);

				while (in_begin != in_end)
				{
					iter_swap(in_begin, out_begin);
					++in_begin;
					++out_begin;
				}
			}

			template <typename I2 = I>
			static constexpr void move_range(I in_begin, I in_end, I2 out_begin)
			{
				GRAILSORT_INVARIANT(in_begin <= in_end);

				while (in_begin != in_end)
				{
					iter_move(in_begin, out_begin);
					++in_begin;
					++out_begin;
				}
			}

			static constexpr void insert_backward(I target, I from)
			{
				GRAILSORT_INVARIANT(target <= from);

				value_type tmp = std::move(*from);
				--from;
				for (; from >= target; --from)
					iter_move(from + 1, from);
				iter_move(target, tmp);
			}

			static constexpr void rotate(I begin, I new_begin, I end)
			{
				GRAILSORT_INVARIANT(begin <= end);
				GRAILSORT_INVARIANT(new_begin <= end);
				GRAILSORT_INVARIANT(begin <= new_begin);

				size_type left_size = distance(begin, new_begin);
				size_type right_size = distance(new_begin, end);
				while (left_size != 0 && right_size != 0)
				{
					if (left_size <= right_size)
					{
						swap_range(begin, begin + left_size, begin + left_size);
						begin += left_size;
						right_size -= left_size;
					}
					else
					{
						swap_range(begin + (left_size - right_size), begin + left_size, begin + left_size);
						left_size -= right_size;
					}
				}
			}

			static constexpr I lower_bound(I begin, I end, I key)
			{
				while (true)
				{
					const size_type d = distance(begin, end);
					if (d <= LINEAR_SEARCH_THRESHOLD)
						break;
					I a = std::next(begin, d / 2);
					if (*a >= *key)
						end = a;
					else
						begin = a + 1;
				}

				while (begin < end && *begin < *key)
					++begin;

				return begin;
			}

			static constexpr I upper_bound(I begin, I end, I key)
			{
				while (true)
				{
					const size_type d = distance(begin, end);
					if (d <= LINEAR_SEARCH_THRESHOLD)
						break;
					I a = std::next(begin, d / 2);
					if (*a > *key)
						end = a;
					else
						begin = a + 1;
				}

				while (begin < end && *begin <= *key)
					++begin;

				return begin;
			}

			static constexpr size_type gather_unique_basic(I begin, I end, size_type expected_unique_count)
			{
				size_type unique_count = 1;
				I internal_buffer_end = begin + 1;
				for (I i = internal_buffer_end; i != end && unique_count != expected_unique_count; ++i)
				{
					const I target = lower_bound(begin, internal_buffer_end, i);
					if (target == internal_buffer_end || *i != *target)
					{
						insert_backward(target, i);
						++unique_count;
						++internal_buffer_end;
					}
				}
				return unique_count;
			}

			static constexpr size_type gather_unique(I begin, I end, size_type expected_unique_count)
			{
				if (distance(begin, end) <= GATHER_UNIQUE_BASIC_THRESHOLD)
					return gather_unique_basic(begin, end, expected_unique_count);

				size_type unique_count = 1;
				I internal_buffer_begin = begin;
				I internal_buffer_end = begin + unique_count;

				for (I i = internal_buffer_end; unique_count != expected_unique_count && i != end; ++i)
				{
					I target = lower_bound(internal_buffer_begin, internal_buffer_end, i);
					if (target == internal_buffer_end || *i != *target)
					{
						const size_type offset = (i - unique_count) - internal_buffer_begin;
						rotate(internal_buffer_begin, internal_buffer_end, i);
						internal_buffer_begin += offset;
						internal_buffer_end += offset;
						target += offset;
						insert_backward(target, i);
						++internal_buffer_end;
						++unique_count;
					}
				}

				rotate(begin, internal_buffer_begin, internal_buffer_end);
				return unique_count;
			}

			static constexpr void internal_merge_forward(I begin, I middle, I end, J buffer_begin)
			{
				I left = begin;
				I right = middle;
				I buffer = buffer_begin;

				while (right < end)
				{
					if (left == middle || *left > *right)
					{
						std::iter_swap(buffer, right);
						++right;
					}
					else
					{
						std::iter_swap(buffer, left);
						++left;
					}
					++buffer;
				}

				if (buffer != left)
					swap_range(buffer, buffer + distance(left, middle), left);
			}

			static constexpr void internal_merge_backward(I begin, I middle, I end, J buffer_end)
			{
				I buffer = buffer_end - 1;
				I right = end - 1;
				I left = middle - 1;

				while (left >= begin)
				{
					if (right < middle || *left > *right)
					{
						std::iter_swap(buffer, left);
						--left;
					}
					else
					{
						std::iter_swap(buffer, right);
						--right;
					}
					--buffer;
				}

				if (right == buffer)
					return;

				while (right >= middle)
				{
					std::iter_swap(buffer, right);
					--buffer;
					--right;
				}
			}

			static constexpr void insertion_sort_basic(I begin, I end)
			{
				for (I i = begin + 1; i < end; ++i)
				{
					value_type tmp = std::move(*i);
					I j = i - 1;
					for (; j >= begin && *j > tmp; --j)
						iter_move(j + 1, j);
					iter_move(j + 1, tmp);
				}
			}

			static constexpr void insertion_sort_unstable(I begin, I end)
			{
				if (distance(begin, end) <= BASIC_INSERTION_SORT_THRESHOLD)
					return insertion_sort_basic(begin, end);
				for (I i = begin + 1; i < end; ++i)
				{
					if (*i < *begin)
						std::iter_swap(begin, i);
					value_type tmp = std::move(*i);
					I j = i - 1;
					for (; *j > tmp; --j)
						iter_move(j + 1, j);
					iter_move(j + 1, tmp);
				}
			}

			static constexpr void insertion_sort_stable(I begin, I end)
			{
				if (distance(begin, end) <= BASIC_INSERTION_SORT_THRESHOLD)
					return insertion_sort_basic(begin, end);
				I min = begin;
				for (I i = min + 1; i < end; ++i)
					if (*i < *min)
						min = i;
				value_type tmp = std::move(*min);
				for (I i = min; i > begin; --i)
					iter_move(i, i - 1);
				new (&*begin) value_type(std::move(tmp));
				for (I i = begin + 1; i < end; ++i)
				{
					tmp = std::move(*i);
					I j = i - 1;
					for (; *j > tmp; --j)
						iter_move(j + 1, j);
					new (&*(j + 1)) value_type(std::move(tmp));
				}
			}

			static constexpr void sort_control_buffer(I begin, I end)
			{
				const size_type size = distance(begin, end);
				if (size > MEDIUM_SORT_THRESHOLD)
					return std::sort(begin, end);
				if (size > SMALL_SORT_THRESHOLD)
					std::make_heap(begin, end, std::greater<>());
				insertion_sort_unstable(begin, end);
			}

			static constexpr void build_small_runs(I begin, I end)
			{
				while (true)
				{
					const I next = begin + SMALL_SORT_THRESHOLD;
					if (next > end)
						return insertion_sort_stable(begin, end);
					insertion_sort_stable(begin, next);
					begin = next;
				}
			}

			static constexpr void lazy_merge(I begin, I middle, I end)
			{
				while (middle != end)
				{
					const I target = upper_bound(begin, middle, end - 1);
					if (target != middle)
					{
						rotate(target, middle, end);
						middle = target;
					}
					if (begin == middle)
						break;
					do
					{
						--end;
					} while (middle != end && *(middle - 1) <= *(end - 1));
				}
			}

			static constexpr void lazy_merge_last(I begin, I middle, I end)
			{
				while (begin != middle)
				{
					const I target = lower_bound(middle, end, begin);
					if (target != middle)
					{
						rotate(begin, middle, target);
						begin += distance(middle, target);
					}
					if (middle == end)
						break;
					do
					{
						begin++;
					} while (begin != middle && *begin <= *middle);
				}
			}

			static constexpr void lazy_merge_pass(I begin, I end, size_type run_size)
			{
				while (true)
				{
					const I merge_middle = begin + run_size;
					if (merge_middle >= end)
						return;
					const I merge_end = merge_middle + run_size;
					if (merge_end > end)
						return lazy_merge_last(begin, merge_middle, end);
					lazy_merge(begin, merge_middle, merge_end);
					begin = merge_end;
				}
			}

			static constexpr void lazy_merge_sort(I begin, I end)
			{
				build_small_runs(begin, end);
				for (size_type run_size = SMALL_SORT_THRESHOLD; run_size < distance(begin, end); run_size *= 2)
					lazy_merge_pass(begin, end, run_size);
			}

			static constexpr void internal_merge_pass(I working_buffer, I begin, I end, size_type run_size)
			{
				while (true)
				{
					const I merge_middle = begin + run_size;
					if (merge_middle >= end)
						break;
					const I merge_end = merge_middle + run_size;
					internal_merge_forward(begin, merge_middle, merge_end, working_buffer);
					working_buffer = merge_middle;
					begin = merge_end;
				}
			}

			static constexpr void build_2sqrt_runs(I working_buffer, I begin, I end, size_type internal_buffer_size)
			{
				size_type run_size = SMALL_SORT_THRESHOLD;
				I local_working_buffer = begin;
				I original_end = end;
				while (run_size < internal_buffer_size)
				{
					const size_type next_run_size = run_size * 2;
					local_working_buffer -= run_size;
					internal_merge_pass(local_working_buffer, begin, end, run_size);
					end -= run_size;
					begin -= run_size;
					run_size = next_run_size;
				}
				rotate(local_working_buffer, end, original_end);
			}

			static constexpr void build_blocks_internal(I control_buffer, I working_buffer, I begin, I end, size_type run_size, size_type block_size)
			{
				while (true)
				{
					I merge_begin = begin;
					I merge_middle = begin + run_size;
					if (merge_middle >= end)
						break;
					I merge_end = merge_middle + run_size;
					if (merge_end > end)
						merge_end = end;

					begin = merge_end;
				}
			}

			static constexpr void merge_blocks(I working_buffer, I begin, I end, size_type size, size_type block_size)
			{
				working_buffer -= block_size;
				internal_merge_pass(working_buffer, begin, end, block_size);
				rotate(working_buffer, end - block_size, end);
			}

			static constexpr void block_merge_sort(I control_buffer, I working_buffer, I begin, I end, size_type size, size_type block_size)
			{
				for (size_type run_size = SMALL_SORT_THRESHOLD; run_size < size; run_size *= 2)
				{
					sort_control_buffer(control_buffer, working_buffer);
					build_blocks_internal(control_buffer, working_buffer, begin, end, run_size, block_size);
					internal_merge_pass(working_buffer, begin, end, block_size);
				}
			}

			static constexpr void insert_internal_buffer(I begin, I internal_buffer_end, I end)
			{
			}

			static constexpr void grail_sort_internal(I begin, I end)
			{
				const size_type size = distance(begin, end);
				if (size <= SMALL_SORT_THRESHOLD)
					return insertion_sort_stable(begin, end);
				const size_type size_sqrt = sqrt_of(size);
				const size_type internal_buffer_size = size_sqrt * 2;
				const size_type unique_count = gather_unique(begin, end, internal_buffer_size);
				if (unique_count != internal_buffer_size)
					return lazy_merge_sort(begin, end);
				const I control_buffer = begin;
				const I working_buffer = begin + size_sqrt;
				const I internal_buffer_end = working_buffer + size_sqrt;
				build_small_runs(internal_buffer_end, end);
				build_2sqrt_runs(working_buffer, internal_buffer_end, end, internal_buffer_size);
				block_merge_sort(control_buffer, working_buffer, internal_buffer_end, end, size, size_sqrt);
				insert_internal_buffer(begin, internal_buffer_end, end);
			}
		};
	}

	template <typename RandomAccessIterator>
	constexpr void sort(RandomAccessIterator begin, RandomAccessIterator end)
	{
		using helper_type = detail::grail_sort_helper<RandomAccessIterator, RandomAccessIterator>;
		helper_type::grail_sort_internal(begin, end);
	}

	template <typename RandomAccessIterator, typename BufferIterator = RandomAccessIterator>
	constexpr void sort(RandomAccessIterator begin, RandomAccessIterator end, BufferIterator external_buffer_begin, BufferIterator external_buffer_end)
	{
		using helper_type = detail::grail_sort_helper<RandomAccessIterator, BufferIterator>;
		helper_type state = {};
		if constexpr (!std::is_void<BufferIterator>::value)
			state.assign_external_buffer(external_buffer_begin, external_buffer_end);
		state.grail_sort_external(begin, end, external_buffer_begin, external_buffer_end);
	}
}