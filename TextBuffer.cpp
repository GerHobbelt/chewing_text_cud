
#include "Base.hpp"

#include "PrivateIntrinsics.hpp"

namespace text_processing {

	// local helper, which knows about our buffersize shenanigans in the TextBuffer class.
	// Hence very local.  ;-)
	static char *alloc_and_copy_string(const char *str, size_t strlength, size_t requested_buffer_size) {
		requested_buffer_size = std::max(strlength + TextBuffer::sentinel_size, requested_buffer_size);
		char *dst = reinterpret_cast<char *>(malloc(requested_buffer_size));
		if (dst == nullptr)
			throw std::bad_alloc();
		memcpy(dst, str, strlength);
		// plant a wide sentinel at the end.
		memset(dst + strlength, '\0', TextBuffer::sentinel_size);
		return dst;
	}

	// local helper, which knows about our buffersize shenanigans in the TextBuffer class.
	// Hence very local.  ;-)
	static char *alloc_bufferspace(size_t length) {
		if (length == 0)
			return nullptr;

		length = std::max(length, 4 * TextBuffer::sentinel_size); // size is a heuristic for 'our guestimate of a *reasonable minimum buffer size*.
		char *dst = reinterpret_cast<char *>(malloc(length));
		if (dst == nullptr)
			throw std::bad_alloc();
		memset(dst, '\0', TextBuffer::sentinel_size);
		return dst;
	}

	TextBuffer::TextBuffer(size_t requested_buffer_size) :
		_capacity(requested_buffer_size),
		_length(0),
		_occupied(0),
		_data(alloc_bufferspace(requested_buffer_size)) {
	}

	TextBuffer::TextBuffer(const char *str) :
		TextBuffer(str, strlen(str)) {
	}

	TextBuffer::TextBuffer(const char *str, size_t length, size_t requested_buffer_size) :
		TextBuffer(std::string_view(str, length), requested_buffer_size) {
	}

	TextBuffer::TextBuffer(const std::string_view &str, size_t requested_buffer_size) :
		// invoke the protected constructor, who does the actual heavy lifting of initializing the instance.
		// 
		// We **intentionally** flipped the order of the parameters so C++ resolution rules don't pick another
		// one and inadvertently produce recursive constructor call code.
		TextBuffer(std::max(str.length() + sentinel_size, requested_buffer_size), str) {
	}

	/* protected */
	TextBuffer::TextBuffer(size_t requested_buffer_size, const std::string_view &str) :
		_length(str.length()),
		_occupied(str.length() + sentinel_size),
		_capacity(requested_buffer_size),
		_data(alloc_and_copy_string(str.data(), str.length(), requested_buffer_size)) {
		assert(_occupied = _length + sentinel_size);
		// ... and check that the text sentinel has been written: a bunch of NULs!
		assert(_data[_occupied - 1] == '\0');
		assert(_data[_occupied - sentinel_size + 1] == '\0');
	}

	// nuke/reset the Textbuffer
	void TextBuffer::clear(void) {
		if (_data != nullptr) {
			free(_data);
		}
		_data = nullptr;
		_length = 0;
		_occupied = 0;
		_capacity = 0;
	}

	TextBuffer::~TextBuffer() {
		clear();
	}

	TextBuffer::TextBuffer(const TextBuffer &src) {
		if (false) std::cout << "copy constructed\n";

		_data = reinterpret_cast<char *>(malloc(src._capacity));
		if (_data == nullptr)
			throw std::bad_alloc();
		_length = src._length;
		_occupied = src._occupied;
		_capacity = src._capacity;
		assert(_length + sentinel_size <= _capacity);
		assert(_length + sentinel_size <= _occupied);
		if (src._occupied)
			memcpy(_data, src._data, src._occupied);  // also copy the sentinel chunk PLUS any data already written in the 'scratch space' beyond:
	}

	TextBuffer::TextBuffer(TextBuffer &&lvsrc):
		_length(std::move(lvsrc._length)),
		_occupied(std::move(lvsrc._occupied)),
		_data(std::move(lvsrc._data)),
		_capacity(std::move(lvsrc._capacity)) {

		if (false) std::cout << "move constructed\n";

		// clear src but DO NOT free src._data as that one was moved into `*this`
		lvsrc._data = nullptr;
		lvsrc._length = 0;
		lvsrc._occupied = 0;
		lvsrc._capacity = 0;
	}

	TextBuffer& TextBuffer::operator=(const TextBuffer& src)
	{
		if (false) std::cout << "copy assigned\n";

		// only alloc the same amount as `src` when nothing has been prepared yet:
		if (_data == nullptr) {
			_data = reinterpret_cast<char *>(malloc(src._capacity));
			if (_data == nullptr)
				throw std::bad_alloc();
			_capacity = src._capacity;
		}
		else if (src._occupied >= _capacity) {
			// redim to make `src` fit anyway.
			auto l = src._occupied;
			free(_data);
			_data = reinterpret_cast<char *>(malloc(l));
			if (_data == nullptr)
				throw std::bad_alloc();
			_capacity = l;
		}

		assert(_capacity >= src._length + sentinel_size);
		assert(_capacity >= src._occupied);
		_length = src._length;
		_occupied = src._occupied;
		if (src._occupied)
			memcpy(_data, src._data, src._occupied);

		return *this;
	}

	TextBuffer& TextBuffer::operator=(TextBuffer&& src)
	{
		if (false) std::cout << "move assigned\n";

		_length = std::move(src._length);
		_occupied = std::move(src._occupied);
		_data = std::move(src._data);
		_capacity = std::move(src._capacity);

		// clear src but DO NOT free src._data as that one was moved into `*this`
		src._data = nullptr;
		src._length = 0;
		src._occupied = 0;
		src._capacity = 0;

		return *this;
	}

	TextBuffer& TextBuffer::operator=(const std::string_view &str) {
		if (false) std::cout << "copy assigned\n";

		// only alloc the same amount as `src` when nothing has been prepared yet:
		if (_data == nullptr) {
			_capacity = str.length() + sentinel_size;
			_data = reinterpret_cast<char *>(malloc(_capacity));
			if (_data == nullptr)
				throw std::bad_alloc();
		} else if (str.length() + sentinel_size >= _capacity) {
			// redim to make `src` fit anyway.
			auto l = str.length() + sentinel_size;
			_data = reinterpret_cast<char *>(realloc(_data, l));
			if (_data == nullptr)
				throw std::bad_alloc();
			_capacity = l;
		}

		assert(_capacity >= str.length() + sentinel_size);
		_length = str.length();
		memcpy(_data, str.data(), _length);
		write_text_edge_sentinel();

		return *this;
	}

	TextBuffer& TextBuffer::operator=(const char *str) {
		if (false) std::cout << "copy assigned\n";

		size_t strlength = strlen(str);

		// only alloc the same amount as `src` when nothing has been prepared yet:
		if (_data == nullptr) {
			_capacity = strlength + sentinel_size;
			_data = reinterpret_cast<char *>(malloc(_capacity));
			if (_data == nullptr)
				throw std::bad_alloc();
		} else if (strlength + sentinel_size >= _capacity) {
			// redim to make `src` fit anyway.
			auto l = strlength + sentinel_size;
			_data = reinterpret_cast<char *>(realloc(_data, l));
			if (_data == nullptr)
				throw std::bad_alloc();
			_capacity = l;
		}

		assert(_capacity >= strlength + sentinel_size);
		_length = strlength;
		memcpy(_data, str, _length);
		write_text_edge_sentinel();

		return *this;
	}

	void TextBuffer::reserve(size_t amount) {
		assert(_data == nullptr);
		assert(_length == 0);
		assert(_occupied == 0);
		assert(_capacity == 0);

		amount += sentinel_size;  // plenty space for sentinels
		_data = reinterpret_cast<char *>(malloc(amount));
		if (_data == nullptr) {
			throw std::bad_alloc();
		}
		_capacity = amount;
	}

	void TextBuffer::reserve(size_t amount, std::error_code &ec) {
		ec.clear();

		assert(_data == nullptr);
		assert(_length == 0);
		assert(_occupied == 0);
		assert(_capacity == 0);

		amount += sentinel_size;  // plenty space for sentinels
		_data = reinterpret_cast<char *>(malloc(amount));
		if (_data == nullptr) {
			//throw std::bad_alloc();
			ec = std::make_error_code(std::errc::not_enough_memory);
			return;
		}
		_capacity = amount;
	}

	void TextBuffer::set_content_size(size_t amount) {
		assert(amount > 0 ? _data != nullptr : true);
		assert(_capacity >= amount + 1);

		_length = amount;
	}

	// write a NUL sentinel of size `sentinel_size` at the end of the 'source string' (which starts at offset 0).
	//
	// This also marks all buffer capacity beyond this point as 'available', i.e. NOT 'occupied'.
	void TextBuffer::write_text_edge_sentinel(void) {
		assert(_length + sentinel_size <= _capacity);
		memset(_data + _length, '\0', sentinel_size);

		_occupied = _length + sentinel_size;
	}

	void TextBuffer::mark_this_space_as_occupied(size_t amount) {
		assert(_occupied + amount <= _capacity);
		_occupied += amount;
	}



	// like assignment operator, but with the option to request additional scratch space by specifying a larger `requested_buffer_size`.
	void TextBuffer::CopyAndPrepare(const char *str, size_t strlength, size_t requested_buffer_size, std::error_code &ec) {
		if (reserve(std::max(requested_buffer_size, strlength), ec), ec) {
			return;
		}
		assert(_capacity >= strlength + sentinel_size);
		memcpy(_data, str, strlength);
		_length = strlength;
		// plant a sentinel at the end.
		write_text_edge_sentinel();
	}

}

