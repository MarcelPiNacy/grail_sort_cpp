#pragma once
#include <utility>
#include <cassert>



namespace detail
{
	template <typename T>
	constexpr void construct(T& object)
	{
		new (object) T();
	}

	template <typename T, typename...U>
	constexpr void construct(T& object, U&&... params)
	{
		new (&object) T(std::forward<U>(params)...);
	}

	template <typename T>
	constexpr void move(T& lhs, T& rhs)
	{
		new (&lhs) T(std::move(rhs));
	}

	template <typename T>
	constexpr void swap(T& lhs, T& rhs)
	{
		T tmp(std::move(lhs));
		move(lhs, rhs));
		move(rhs, tmp));
	}

	template <typename T>
	constexpr void conditional_swap(bool condition, T& lhs, T& rhs)
	{
		if constexpr (std::is_trivial<T>)
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
		T tmp;
		while (lhs != lhs + size)
		{
			move(tmp, *lhs);
			move(*lhs, *rhs);
			move(*rhs, tmp);
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
	constexpr U lower_bound(T* array, U begin, U end, const T& key)
	{
		U low = begin;
		U high = end;
		while (low < high)
		{
			const U p = (low + high) / 2;
			const bool flag = array[low] >= key;
			if (flag)
				high = p;
			if (!flag)
				low = p;
		}
		return high;
	}

	template <typename T, typename U = size_t>
	constexpr U upper_bound(T* array, U begin, U end, const T& key)
	{
		U low = begin;
		U high = end;
		while (low < high)
		{
			const U p = (low + high) / 2;
			const bool flag = array[low] >= key;
			if (flag)
				high = p;
			if (!flag)
				low = p;
		}
		return high;
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
	constexpr void build_blocks(T* array, U size, T* external_buffer, U external_buffer_size)
	{
	}

	template <typename T, typename U = size_t>
	constexpr void lazy_merge(T* ptr, U left_size, U right_size)
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
			for (U i = 0; i < size - next; i += next)
				lazy_merge(array + i, merge_size, merge_size);
			merge_size = next;
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

constexpr size_t GRAIL_SORT_STATIC_BUFFER_SIZE = 512;

enum class grail_sort_buffer_type
{
	STATIC,
	NONE,
	DYNAMIC,
};

struct grail_sort_dynamic_buffer_options
{
	size_t dynamic_buffer_size;
	void* (*allocate)(size_t size);
	void (*deallocate)(void* buffer, size_t size);
};

struct grail_sort_options
{
	grail_sort_buffer_type buffer_type;
	const grail_sort_dynamic_buffer_options* dynamic_buffer_options;
};

template <typename T, typename U = size_t>
constexpr void grail_sort(T* array, U size, grail_sort_options options = {})
{
	if (size < 16)
		return detail::insertion_sort(array, size);

	switch (options.buffer_type)
	{
	case grail_sort_buffer_type::STATIC:
		{
			constexpr size_t K = GRAIL_SORT_STATIC_BUFFER_SIZE;
			T buffer[K];
			detail::grail_sort_entry_point(array, size, buffer, K);
		}
		break;
	case grail_sort_buffer_type::NONE:
		detail::grail_sort_entry_point(array, size, nullptr, 0);
		break;
	case grail_sort_buffer_type::DYNAMIC:
		{
			assert(options.dynamic_buffer_options != nullptr);
			const size_t buffer_size = options.dynamic_buffer_options->dynamic_buffer_size;
			const size_t allocation_size = buffer_size * sizeof(T);
			T* buffer = options.dynamic_buffer_options->allocate(allocation_size);
			assert(buffer != nullptr);
			detail::grail_sort_entry_point(array, size, buffer, buffer_size);
			options.dynamic_buffer_options->deallocate(buffer, allocation_size);
		}
		break;
	default:
		assert(false);
		break;
	}
}