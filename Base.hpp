#pragma once

#include <expected>
#include <string>
#include <vector>
#include <filesystem>
#include <system_error>
#include <format>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>
#include <type_traits>

#if defined(_MSC_VER)
#include <sal.h>
#endif

#include <libassert/assert.h>

namespace text_processing {

	using std::filesystem::path;

	struct ErrorResponse {
		std::errc code;
		std::string message;
	};

	typedef std::vector<path> searchPaths;

	std::expected<path, ErrorResponse> locateFile(const path &filepath, const path &source_filepath = std::filesystem::current_path(), const searchPaths& search_paths = {}, bool specfile_path_is_also_search_path = true, bool accept_absolute_paths = true, bool accept_relative_paths = true);

	// Return a relative path, that's relative to the given `base_filepath`.
	std::expected<path, ErrorResponse> ConvertToRelativePath(const path &filepath, const path &base_filepath = std::filesystem::current_path());

	// Return a canonical=normalized absolute path for `filepath`. When the `filepath` is relative, it is based relative to the `base_filepath`, which, when not an absolute path itself, is based relative to the `current_working_directory` path (which, when empty, assumes `fs::current_path()`).
	//
	// Simplified: retval = current_working_directory / base_filepath / filepath
	path ConvertToAbsoluteNormalizedPath(const path &filepath, const path &base_filepath, const path &current_working_directory = std::filesystem::current_path());

	path NormalizePathToUnixSeparators(const path &filepath);

	// ---------------------------------------------------------------

	template <class _Traits>
	class string_view_iterator {
	public:
		using value_type        = typename _Traits::char_type;
		using size_type         = size_t;
		using difference_type   = ptrdiff_t;
		using pointer           = const value_type*;
		using reference         = const value_type&;

		constexpr string_view_iterator() noexcept = default;

	private:
		friend string_view<value_type, _Traits>;

		constexpr string_view_iterator(const pointer data_arg, const size_type size_arg, const size_type offset_arg) noexcept
			: _data(data_arg), _size(size_arg), _offset(offset_arg) {
		}

	public:
		constexpr reference operator*() const noexcept {
			LIBASSERT_DEBUG_ASSERT(_data, "cannot dereference value-initialized string_view iterator");
			LIBASSERT_DEBUG_ASSERT(_offset < _size, "cannot dereference end string_view iterator");
			return _data[_offset];
		}

		constexpr pointer operator->() const noexcept {
			LIBASSERT_DEBUG_ASSERT(_data, "cannot dereference value-initialized string_view iterator");
			LIBASSERT_DEBUG_ASSERT(_offset < _size, "cannot dereference end string_view iterator");
			return _data + _offset;
		}

		constexpr string_view_iterator& operator++() noexcept {
			LIBASSERT_DEBUG_ASSERT(_data, "cannot increment value-initialized string_view iterator");
			LIBASSERT_DEBUG_ASSERT(_offset < _size, "cannot increment string_view iterator past end");
			++_offset;
			return *this;
		}

		constexpr string_view_iterator operator++(int) noexcept {
			string_view_iterator _Tmp{*this};
			++*this;
			return _Tmp;
		}

		constexpr string_view_iterator& operator--() noexcept {
			LIBASSERT_DEBUG_ASSERT(_data, "cannot decrement value-initialized string_view iterator");
			LIBASSERT_DEBUG_ASSERT(_offset != 0, "cannot decrement string_view iterator before begin");
			--_offset;
			return *this;
		}

		constexpr string_view_iterator operator--(int) noexcept {
			string_view_iterator _Tmp{*this};
			--*this;
			return _Tmp;
		}

#if LIBASSERT_DO_ASSERTIONS
		constexpr void VERIFY_OFFSET(const difference_type offset_arg) const noexcept {
			if (offset_arg != 0) {
				LIBASSERT_DEBUG_ASSERT(_data, "cannot seek value-initialized string_view iterator");
			}

			if (offset_arg < 0) {
				LIBASSERT_DEBUG_ASSERT(_offset >= size_type{0} - static_cast<size_type>(offset_arg), "cannot seek string_view iterator before begin");
			}

			if (offset_arg > 0) {
				LIBASSERT_DEBUG_ASSERT(_size - _offset >= static_cast<size_type>(offset_arg), "cannot seek string_view iterator after end");
			}
		}
#else
		#define VERIFY_OFFSET(offset_arg)    ((void)0)
#endif

		constexpr string_view_iterator& operator+=(const difference_type offset_arg) noexcept {
			VERIFY_OFFSET(offset_arg);
			_offset += static_cast<size_type>(offset_arg);

			return *this;
		}

		constexpr string_view_iterator operator+(const difference_type offset_arg) const noexcept {
			string_view_iterator _Tmp{*this};
			_Tmp += offset_arg;
			return _Tmp;
		}

		friend constexpr string_view_iterator operator+(
			const difference_type offset_arg, string_view_iterator _Right) noexcept {
			_Right += offset_arg;
			return _Right;
		}

		constexpr string_view_iterator& operator-=(const difference_type offset_arg) noexcept {
			if (offset_arg != 0) {
				LIBASSERT_DEBUG_ASSERT(_data, "cannot seek value-initialized string_view iterator");
			}

			if (offset_arg > 0) {
				LIBASSERT_DEBUG_ASSERT(_offset >= static_cast<size_type>(offset_arg), "cannot seek string_view iterator before begin");
			}

			if (offset_arg < 0) {
				LIBASSERT_DEBUG_ASSERT(_size - _offset >= size_type{0} - static_cast<size_type>(offset_arg),
					"cannot seek string_view iterator after end");
			}

			_offset -= static_cast<size_type>(offset_arg);

			return *this;
		}

		constexpr string_view_iterator operator-(const difference_type offset_arg) const noexcept {
			string_view_iterator _Tmp{*this};
			_Tmp -= offset_arg;
			return _Tmp;
		}

		constexpr difference_type operator-(const string_view_iterator& _Right) const noexcept {
			LIBASSERT_DEBUG_ASSERT(_data == _Right._data && _size == _Right._size,
				"cannot subtract incompatible string_view iterators");
			return static_cast<difference_type>(_offset - _Right._offset);
		}

		constexpr reference operator[](const difference_type offset_arg) const noexcept {
			return *(*this + offset_arg);
		}

		constexpr bool operator==(const string_view_iterator& _Right) const noexcept {
			LIBASSERT_DEBUG_ASSERT(_data == _Right._data && _size == _Right._size,
				"cannot compare incompatible string_view iterators for equality");
			return _offset == _Right._offset;
		}

		constexpr strong_ordering operator<=>(const string_view_iterator& _Right) const noexcept {
			LIBASSERT_DEBUG_ASSERT(_data == _Right._data && _size == _Right._size,
				"cannot compare incompatible string_view iterators");
			return _offset <=> _Right._offset;
		}

		constexpr bool operator!=(const string_view_iterator& _Right) const noexcept {
			return !(*this == _Right);
		}

		constexpr bool operator<(const string_view_iterator& _Right) const noexcept {
			LIBASSERT_DEBUG_ASSERT(_data == _Right._data && _size == _Right._size,
				"cannot compare incompatible string_view iterators");
			return _offset < _Right._offset;
		}

		constexpr bool operator>(const string_view_iterator& _Right) const noexcept {
			return _Right < *this;
		}

		constexpr bool operator<=(const string_view_iterator& _Right) const noexcept {
			return !(_Right < *this);
		}

		constexpr bool operator>=(const string_view_iterator& _Right) const noexcept {
			return !(*this < _Right);
		}

		friend constexpr void _Verify_range(const string_view_iterator& _First, const string_view_iterator& _Last) {
			LIBASSERT_DEBUG_ASSERT(_First._data == _Last._data && _First._size == _Last._size,
				"string_view iterators in range are from different views");
			LIBASSERT_DEBUG_ASSERT(_First._offset <= _Last._offset, "string_view iterator range transposed");
		}

		using _Prevent_inheriting_unwrap = string_view_iterator;

	private:
		pointer _data = nullptr;
		size_type _size  = 0;
		size_type _offset   = 0;
	};


	template <class _Elem, class _Traits>
		class string_view { // non-owning wrapper for any kind of contiguous character buffer
		public:
			static_assert(std::is_same_v<_Elem, typename _Traits::char_type>,
				"Bad char_traits for string_view; N4950 [string.view.template.general]/1 "
				"\"The program is ill-formed if traits::char_type is not the same type as charT.\"");

			static_assert(!std::is_array_v<_Elem> && std::is_trivially_copyable_v<_Elem> && std::is_trivially_default_constructible_v<_Elem>
							  && std::is_standard_layout_v<_Elem>,
				"The character type of string_view must be a non-array trivially copyable standard-layout type T where "
				"is_trivially_default_constructible_v<T> is true. See N5001 [strings.general]/1.");

			using traits_type            = _Traits;
			using value_type             = _Elem;
			using pointer                = _Elem*;
			using const_pointer          = const _Elem*;
			using reference              = _Elem&;
			using const_reference        = const _Elem&;
			using const_iterator         = string_view_iterator<_Traits>;
			using iterator               = const_iterator;
			using const_reverse_iterator = std::reverse_iterator<const_iterator>;
			using reverse_iterator       = const_reverse_iterator;
			using size_type              = size_t;
			using difference_type        = ptrdiff_t;

			static constexpr auto npos{static_cast<size_type>(-1)};

			constexpr string_view() noexcept: _data(), _size(0) {}

			constexpr string_view(const string_view&) noexcept            = default;
			constexpr string_view& operator=(const string_view&) noexcept = default;

			/* implicit */ constexpr string_view(_In_z_ const const_pointer _Ntcts) noexcept // strengthened
				: _data(_Ntcts), _size(_Traits::length(_Ntcts)) {
			}

			string_view(nullptr_t) = delete;

			constexpr string_view(
				_In_reads_(_Count) const const_pointer _Cts, const size_type _Count) noexcept // strengthened
				: _data(_Cts), _size(_Count) {
				LIBASSERT_DEBUG_ASSERT(_Count == 0 || _Cts, "cannot construct a string_view from a null pointer and a non-zero size");
			}

			template <contiguous_iterator _Iter, sized_sentinel_for<_Iter> _Sent>
				requires (is_same_v<iter_value_t<_Iter>, _Elem> && !is_convertible_v<_Sent, size_type>)
			constexpr string_view(_Iter _First, _Sent _Last) noexcept(noexcept(_Last - _First)) // strengthened
				: _data(std::to_address(_First)), _size(static_cast<size_type>(_Last - _First)) {
			}

			template <class _Range>
				requires (!same_as<remove_cvref_t<_Range>, string_view> && std::ranges::contiguous_range<_Range>
			&& std::ranges::sized_range<_Range> && same_as<std::ranges::range_value_t<_Range>, _Elem>
				&& !is_convertible_v<_Range, const _Elem*>
				&& !requires(
					remove_cvref_t<_Range>& _Rng) {
				_Rng.operator std::string_view<_Elem, _Traits>();
			})
				constexpr explicit string_view(_Range&& _Rng)
				noexcept(noexcept(std::ranges::data(_Rng)) && noexcept(std::ranges::size(_Rng))) // strengthened
				: _data(std::ranges::data(_Rng)), _size(static_cast<size_type>(std::ranges::size(_Rng))) {
			}

			constexpr const_iterator begin() const noexcept {
				return const_iterator(_data, _size, 0);
			}

			constexpr const_iterator end() const noexcept {
				return const_iterator(_data, _size, _size);
			}

			constexpr const_iterator cbegin() const noexcept {
				return begin();
			}

			constexpr const_iterator cend() const noexcept {
				return end();
			}

			constexpr const_reverse_iterator rbegin() const noexcept {
				return const_reverse_iterator{end()};
			}

			constexpr const_reverse_iterator rend() const noexcept {
				return const_reverse_iterator{begin()};
			}

			constexpr const_reverse_iterator crbegin() const noexcept {
				return rbegin();
			}

			constexpr const_reverse_iterator crend() const noexcept {
				return rend();
			}

			constexpr const_pointer _Unchecked_begin() const noexcept {
				return _data;
			}

			constexpr const_pointer _Unchecked_end() const noexcept {
				return _data + _size;
			}

			constexpr size_type size() const noexcept {
				return _size;
			}

			constexpr size_type length() const noexcept {
				return _size;
			}

			constexpr bool empty() const noexcept {
				return _size == 0;
			}

			constexpr const_pointer data() const noexcept {
				return _data;
			}

			constexpr size_type max_size() const noexcept {
				// bound to PTRDIFF_MAX to make end() - begin() well defined (also makes room for npos)
				// bound to static_cast<size_type>(-1) / sizeof(_Elem) by address space limits
				return (std::min)(static_cast<size_type>(PTRDIFF_MAX), static_cast<size_type>(-1) / sizeof(_Elem));
			}

			constexpr const_reference operator[](const size_type offset_arg) const noexcept /* strengthened */ {
				LIBASSERT_DEBUG_ASSERT(offset_arg < _size, "string_view subscript out of range");

				// CodeQL [SM01954] This index is optionally validated above.
				return _data[offset_arg];
			}

			constexpr const_reference at(const size_type offset_arg) const {
				// get the character at offset_arg or throw if that is out of range
				_Check_offset_exclusive(offset_arg);
				return _data[offset_arg];
			}

			constexpr const_reference front() const noexcept /* strengthened */ {
				LIBASSERT_DEBUG_ASSERT(_size != 0, "front() called on empty string_view");

				return _data[0];
			}

			constexpr const_reference back() const noexcept /* strengthened */ {
				LIBASSERT_DEBUG_ASSERT(_size != 0, "back() called on empty string_view");

				return _data[_size - 1];
			}

			constexpr void remove_prefix(const size_type _Count) noexcept /* strengthened */ {
				LIBASSERT_DEBUG_ASSERT(_size >= _Count, "cannot remove_prefix() larger than string_view size");

				_data += _Count;
				_size -= _Count;
			}

			constexpr void remove_suffix(const size_type _Count) noexcept /* strengthened */ {
				LIBASSERT_DEBUG_ASSERT(_size >= _Count, "cannot remove_suffix() larger than string_view size");

				_size -= _Count;
			}

			constexpr void swap(string_view& _Other) noexcept {
				const string_view _Tmp{_Other}; // note: std::swap is not constexpr before C++20
				_Other = *this;
				*this  = _Tmp;
			}

			constexpr size_type copy(
				_Out_writes_(_Count) _Elem* const _Ptr, size_type _Count, const size_type offset_arg = 0) const {
				// copy [offset_arg, offset_arg + Count) to [_Ptr, _Ptr + _Count)
				_Check_offset(offset_arg);
				_Count = _Clamp_suffix_size(offset_arg, _Count);
				_Traits::copy(_Ptr, _data + offset_arg, _Count);
				return _Count;
			}

			_Pre_satisfies_(_Dest_size >= _Count) constexpr size_type
				_Copy_s(_Out_writes_all_(_Dest_size) _Elem* const _Dest, const size_type _Dest_size, size_type _Count,
					const size_type offset_arg = 0) const {
				// copy [offset_arg, offset_arg + _Count) to [_Dest, _Dest + _Count)
				_Check_offset(offset_arg);
				_Count = _Clamp_suffix_size(offset_arg, _Count);
				_Traits::_Copy_s(_Dest, _Dest_size, _data + offset_arg, _Count);
				return _Count;
			}

			constexpr string_view substr(const size_type offset_arg = 0, size_type _Count = npos) const {
				// return a new string_view moved forward by offset_arg and trimmed to _Count elements
				_Check_offset(offset_arg);
				_Count = _Clamp_suffix_size(offset_arg, _Count);
				return string_view(_data + offset_arg, _Count);
			}

			constexpr bool _Equal(const string_view _Right) const noexcept {
				return _Traits_equal<_Traits>(_data, _size, _Right._data, _Right._size);
			}

			constexpr int compare(const string_view _Right) const noexcept {
				return _Traits_compare<_Traits>(_data, _size, _Right._data, _Right._size);
			}

			constexpr int compare(const size_type offset_arg, const size_type _Nx, const string_view _Right) const {
				// compare [offset_arg, offset_arg + _Nx) with _Right
				return substr(offset_arg, _Nx).compare(_Right);
			}

			constexpr int compare(const size_type offset_arg, const size_type _Nx, const string_view _Right,
				const size_type _Roff, const size_type _Count) const {
				// compare [offset_arg, offset_arg + _Nx) with _Right [_Roff, _Roff + _Count)
				return substr(offset_arg, _Nx).compare(_Right.substr(_Roff, _Count));
			}

			constexpr int compare(_In_z_ const _Elem* const _Ptr) const noexcept /* strengthened */ {
				// compare [0, _size) with [_Ptr, <null>)
				return compare(string_view(_Ptr));
			}

			constexpr int compare(const size_type offset_arg, const size_type _Nx, _In_z_ const _Elem* const _Ptr) const {
				// compare [offset_arg, offset_arg + _Nx) with [_Ptr, <null>)
				return substr(offset_arg, _Nx).compare(string_view(_Ptr));
			}

			constexpr int compare(const size_type offset_arg, const size_type _Nx,
				_In_reads_(_Count) const _Elem* const _Ptr, const size_type _Count) const {
				// compare [offset_arg, offset_arg + _Nx) with [_Ptr, _Ptr + _Count)
				return substr(offset_arg, _Nx).compare(string_view(_Ptr, _Count));
			}

			constexpr bool starts_with(const string_view _Right) const noexcept {
				const auto _Rightsize = _Right._size;
				if (_size < _Rightsize) {
					return false;
				}
				return _Traits::compare(_data, _Right._data, _Rightsize) == 0;
			}

			constexpr bool starts_with(const _Elem _Right) const noexcept {
				return !empty() && _Traits::eq(front(), _Right);
			}

			constexpr bool starts_with(const _Elem* const _Right) const noexcept /* strengthened */ {
				return starts_with(string_view(_Right));
			}

			constexpr bool ends_with(const string_view _Right) const noexcept {
				const auto _Rightsize = _Right._size;
				if (_size < _Rightsize) {
					return false;
				}
				return _Traits::compare(_data + (_size - _Rightsize), _Right._data, _Rightsize) == 0;
			}

			constexpr bool ends_with(const _Elem _Right) const noexcept {
				return !empty() && _Traits::eq(back(), _Right);
			}

			constexpr bool ends_with(const _Elem* const _Right) const noexcept /* strengthened */ {
				return ends_with(string_view(_Right));
			}

			constexpr bool contains(const string_view _Right) const noexcept {
				return find(_Right) != npos;
			}

			constexpr bool contains(const _Elem _Right) const noexcept {
				return find(_Right) != npos;
			}

			constexpr bool contains(const _Elem* const _Right) const noexcept /* strengthened */ {
				return find(_Right) != npos;
			}

			constexpr size_type find(const string_view _Right, const size_type offset_arg = 0) const noexcept {
				// look for _Right beginning at or after offset_arg
				return _Traits_find<_Traits>(_data, _size, offset_arg, _Right._data, _Right._size);
			}

			constexpr size_type find(const _Elem _Ch, const size_type offset_arg = 0) const noexcept {
				// look for _Ch at or after offset_arg
				return _Traits_find_ch<_Traits>(_data, _size, offset_arg, _Ch);
			}

			constexpr size_type find(_In_reads_(_Count) const _Elem* const _Ptr, const size_type offset_arg,
				const size_type _Count) const noexcept /* strengthened */ {
				// look for [_Ptr, _Ptr + _Count) beginning at or after offset_arg
				return _Traits_find<_Traits>(_data, _size, offset_arg, _Ptr, _Count);
			}

			constexpr size_type find(_In_z_ const _Elem* const _Ptr, const size_type offset_arg = 0) const noexcept
				/* strengthened */ {
				// look for [_Ptr, <null>) beginning at or after offset_arg
				return _Traits_find<_Traits>(_data, _size, offset_arg, _Ptr, _Traits::length(_Ptr));
			}

			constexpr size_type rfind(const string_view _Right, const size_type offset_arg = npos) const noexcept {
				// look for _Right beginning before offset_arg
				return _Traits_rfind<_Traits>(_data, _size, offset_arg, _Right._data, _Right._size);
			}

			constexpr size_type rfind(const _Elem _Ch, const size_type offset_arg = npos) const noexcept {
				// look for _Ch before offset_arg
				return _Traits_rfind_ch<_Traits>(_data, _size, offset_arg, _Ch);
			}

			constexpr size_type rfind(_In_reads_(_Count) const _Elem* const _Ptr, const size_type offset_arg,
				const size_type _Count) const noexcept /* strengthened */ {
				// look for [_Ptr, _Ptr + _Count) beginning before offset_arg
				return _Traits_rfind<_Traits>(_data, _size, offset_arg, _Ptr, _Count);
			}

			constexpr size_type rfind(_In_z_ const _Elem* const _Ptr, const size_type offset_arg = npos) const noexcept
				/* strengthened */ {
				// look for [_Ptr, <null>) beginning before offset_arg
				return _Traits_rfind<_Traits>(_data, _size, offset_arg, _Ptr, _Traits::length(_Ptr));
			}

			constexpr size_type find_first_of(const string_view _Right,
				const size_type offset_arg = 0) const noexcept { // look for one of _Right at or after offset_arg
				return _Traits_find_first_of<_Traits>(_data, _size, offset_arg, _Right._data, _Right._size);
			}

			constexpr size_type find_first_of(const _Elem _Ch, const size_type offset_arg = 0) const noexcept {
				// look for _Ch at or after offset_arg
				return _Traits_find_ch<_Traits>(_data, _size, offset_arg, _Ch);
			}

			constexpr size_type find_first_of(_In_reads_(_Count) const _Elem* const _Ptr, const size_type offset_arg,
				const size_type _Count) const noexcept /* strengthened */ {
				// look for one of [_Ptr, _Ptr + _Count) at or after offset_arg
				return _Traits_find_first_of<_Traits>(_data, _size, offset_arg, _Ptr, _Count);
			}

			constexpr size_type find_first_of(
				_In_z_ const _Elem* const _Ptr, const size_type offset_arg = 0) const noexcept /* strengthened */ {
				// look for one of [_Ptr, <null>) at or after offset_arg
				return _Traits_find_first_of<_Traits>(_data, _size, offset_arg, _Ptr, _Traits::length(_Ptr));
			}

			constexpr size_type find_last_of(const string_view _Right,
				const size_type offset_arg = npos) const noexcept { // look for one of _Right before offset_arg
				return _Traits_find_last_of<_Traits>(_data, _size, offset_arg, _Right._data, _Right._size);
			}

			constexpr size_type find_last_of(const _Elem _Ch, const size_type offset_arg = npos) const noexcept {
				// look for _Ch before offset_arg
				return _Traits_rfind_ch<_Traits>(_data, _size, offset_arg, _Ch);
			}

			constexpr size_type find_last_of(_In_reads_(_Count) const _Elem* const _Ptr, const size_type offset_arg,
				const size_type _Count) const noexcept /* strengthened */ {
				// look for one of [_Ptr, _Ptr + _Count) before offset_arg
				return _Traits_find_last_of<_Traits>(_data, _size, offset_arg, _Ptr, _Count);
			}

			constexpr size_type find_last_of(
				_In_z_ const _Elem* const _Ptr, const size_type offset_arg = npos) const noexcept /* strengthened */ {
				// look for one of [_Ptr, <null>) before offset_arg
				return _Traits_find_last_of<_Traits>(_data, _size, offset_arg, _Ptr, _Traits::length(_Ptr));
			}

			constexpr size_type find_first_not_of(const string_view _Right,
				const size_type offset_arg = 0) const noexcept { // look for none of _Right at or after offset_arg
				return _Traits_find_first_not_of<_Traits>(_data, _size, offset_arg, _Right._data, _Right._size);
			}

			constexpr size_type find_first_not_of(const _Elem _Ch, const size_type offset_arg = 0) const noexcept {
				// look for any value other than _Ch at or after offset_arg
				return _Traits_find_not_ch<_Traits>(_data, _size, offset_arg, _Ch);
			}

			constexpr size_type find_first_not_of(_In_reads_(_Count) const _Elem* const _Ptr, const size_type offset_arg,
				const size_type _Count) const noexcept /* strengthened */ {
				// look for none of [_Ptr, _Ptr + _Count) at or after offset_arg
				return _Traits_find_first_not_of<_Traits>(_data, _size, offset_arg, _Ptr, _Count);
			}

			constexpr size_type find_first_not_of(
				_In_z_ const _Elem* const _Ptr, const size_type offset_arg = 0) const noexcept /* strengthened */ {
				// look for none of [_Ptr, <null>) at or after offset_arg
				return _Traits_find_first_not_of<_Traits>(_data, _size, offset_arg, _Ptr, _Traits::length(_Ptr));
			}

			constexpr size_type find_last_not_of(const string_view _Right,
				const size_type offset_arg = npos) const noexcept { // look for none of _Right before offset_arg
				return _Traits_find_last_not_of<_Traits>(_data, _size, offset_arg, _Right._data, _Right._size);
			}

			constexpr size_type find_last_not_of(const _Elem _Ch, const size_type offset_arg = npos) const noexcept {
				// look for any value other than _Ch before offset_arg
				return _Traits_rfind_not_ch<_Traits>(_data, _size, offset_arg, _Ch);
			}

			constexpr size_type find_last_not_of(_In_reads_(_Count) const _Elem* const _Ptr, const size_type offset_arg,
				const size_type _Count) const noexcept /* strengthened */ {
				// look for none of [_Ptr, _Ptr + _Count) before offset_arg
				return _Traits_find_last_not_of<_Traits>(_data, _size, offset_arg, _Ptr, _Count);
			}

			constexpr size_type find_last_not_of(
				_In_z_ const _Elem* const _Ptr, const size_type offset_arg = npos) const noexcept /* strengthened */ {
				// look for none of [_Ptr, <null>) before offset_arg
				return _Traits_find_last_not_of<_Traits>(_data, _size, offset_arg, _Ptr, _Traits::length(_Ptr));
			}

			constexpr bool starts_with(const string_view view_arg) const noexcept {
				return _size >= view_arg._size && _Traits::compare(_data, view_arg._data, view_arg._size) == 0;
			}

		private:
			constexpr void _Check_offset(const size_type offset_arg) const { // checks whether offset_arg is in the bounds of [0, size()]
				if (_size < offset_arg) {
					_Xout_of_range("invalid string_view position");
				}
			}

			constexpr void _Check_offset_exclusive(const size_type offset_arg) const {
				// checks whether offset_arg is in the bounds of [0, size())
				if (_size <= offset_arg) {
					_Xout_of_range("invalid string_view position");
				}
			}

			constexpr size_type _Clamp_suffix_size(const size_type offset_arg, const size_type size_arg) const noexcept {
				// trims size_arg to the longest it can be assuming a string at/after offset_arg
				return (std::min)(size_arg, _size - offset_arg);
			}

			const_pointer _data;
			size_type _size;
	};


















	// ---------------------------------------------------------------

	class TextBuffer {
	public:
		using size_type         = size_t;
		using difference_type   = ptrdiff_t;

	protected:
		char *_data = nullptr;
		size_type _length = 0;     // length of the 'source text' (which starts at buffer offset 0)
		size_type _occupied = 0;	// any bytes in the buffer (capacity) are available for allocation.
		size_type _capacity = 0;

	public:
		static constexpr const size_type sentinel_size = 32;

		//TextBuffer() = default;
		TextBuffer(const char *str);
		TextBuffer(const char *str, size_type length, size_type requested_buffer_size = 0);
		TextBuffer(const std::string_view &str, size_type requested_buffer_size = 0);
		TextBuffer(size_type requested_buffer_size = 0);
	protected:
		// helper for the other constructors: code deduplication
		explicit TextBuffer(size_type requested_buffer_size, const std::string_view &str);
	public:

		~TextBuffer();

		TextBuffer(const TextBuffer &src);
		TextBuffer(TextBuffer &&lvsrc);

		TextBuffer& operator=(const TextBuffer& other);
		TextBuffer& operator=(TextBuffer&& other);

		TextBuffer& operator=(const std::string_view &str);
		TextBuffer& operator=(const char *str);

		void reserve(size_type amount);
		void reserve(size_type amount, std::error_code &ec);

		constexpr char *data() const {
			return _data;
		}
		constexpr size_type content_length() const {
			return _length;
		}
		constexpr size_type available_space() const {
			assert(_occupied >= _length + sentinel_size);
			return _capacity - _occupied;
		}
		constexpr size_type capacity() const {
			return _capacity;
		}
		constexpr std::string_view content_view() const {
			return {_data, _length};
		}
		constexpr std::string_view available_space_view() {
			return {_data, _length};
		}
		constexpr std::string_view capacity_view() const {
			return {_data, _capacity};
		}

		// write a NUL sentinel of size `sentinel_size` at the end of the 'source string' (which starts at offset 0).
		//
		// This also marks all buffer capacity beyond this point as 'available', i.e. NOT 'occupied'.
		void write_text_edge_sentinel(void);

		void set_content_size(size_type amount);

		void mark_this_space_as_occupied(size_type amount);

		// like assignment operator, but with the option to request additional scratch space by specifying a larger `requested_buffer_size`.
		void CopyAndPrepare(const char *str, size_type strlength, size_type requested_buffer_size, std::error_code &ec);

		// nuke/reset the Textbuffer
		void clear(void);
	};



}

