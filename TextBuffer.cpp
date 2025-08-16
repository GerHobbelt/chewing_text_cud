
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

		length = std::max(length, TextBuffer::sentinel_size);
		char *dst = reinterpret_cast<char *>(malloc(length));
		if (dst == nullptr)
			throw std::bad_alloc();
		memset(dst, '\0', TextBuffer::sentinel_size);
		return dst;
	}

	TextBuffer::TextBuffer(size_t requested_buffer_size) :
		_capacity(requested_buffer_size),
		_size(0),
		_data(alloc_bufferspace(requested_buffer_size)) {
	}

	TextBuffer::TextBuffer(const char *str) :
		TextBuffer(str, strlen(str)) {
	}

	TextBuffer::TextBuffer(const char *str, size_t length, size_t requested_buffer_size) :
		TextBuffer(std::string_view(str, length), requested_buffer_size) {
	}

	TextBuffer::TextBuffer(const std::string_view &str, size_t requested_buffer_size) :
		TextBuffer(std::max(str.length() + sentinel_size, requested_buffer_size), str) {
	}

	/* protected */
	TextBuffer::TextBuffer(size_t requested_buffer_size, const std::string_view &str) :
		_size(str.length()),
		_capacity(requested_buffer_size),
		_data(alloc_and_copy_string(str.data(), str.length(), requested_buffer_size)) {
	}

	// nuke/reset the Textbuffer
	void TextBuffer::clear(void) {
		if (_data != nullptr) {
			free(_data);
		}
		_data = nullptr;
		_size = 0;
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
		_size = src._size;
		_capacity = src._capacity;
		assert(_size + sentinel_size <= _capacity);
		memcpy(_data, src._data, src._size + sentinel_size);  // also copy the sentinel chunk
	}

	TextBuffer::TextBuffer(TextBuffer &&lvsrc):
		_size(std::move(lvsrc._size)),
		_data(std::move(lvsrc._data)),
		_capacity(std::move(lvsrc._capacity)) {

		if (false) std::cout << "move constructed\n";

		// clear src but DO NOT free src._data as that one was moved into `*this`
		lvsrc._data = nullptr;
		lvsrc._size = 0;
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
		else if (src._size >= _capacity) {
			// redim to make `src` fit anyway.
			auto l = src._size + sentinel_size;
			free(_data);
			_data = reinterpret_cast<char *>(malloc(l));
			if (_data == nullptr)
				throw std::bad_alloc();
			_capacity = l;
		}

		assert(_capacity >= src._size + sentinel_size);
		_size = src._size;
		memcpy(_data, src._data, src._size);
		write_buffer_edge_sentinel();

		return *this;
	}

	TextBuffer& TextBuffer::operator=(TextBuffer&& src)
	{
		if (false) std::cout << "move assigned\n";

		_size = std::move(src._size);
		_data = std::move(src._data);
		_capacity = std::move(src._capacity);

		// clear src but DO NOT free src._data as that one was moved into `*this`
		src._data = nullptr;
		src._size = 0;
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
			free(_data);
			_data = reinterpret_cast<char *>(malloc(l));
			if (_data == nullptr)
				throw std::bad_alloc();
			_capacity = l;
		}

		assert(_capacity >= str.length() + sentinel_size);
		_size = str.length();
		memcpy(_data, str.data(), _size);
		write_buffer_edge_sentinel();

		return *this;
	}


	void TextBuffer::reserve(size_t amount) {
		assert(_data == nullptr);
		assert(_size == 0);
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
		assert(_size == 0);
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

		_size = amount;
	}

	void TextBuffer::write_buffer_edge_sentinel(void) {
		assert(_size + sentinel_size <= _capacity);
		memset(_data + _size, '\0', sentinel_size);
	}

	void TextBuffer::CopyAndPrepare(const char *str, size_t strlength, size_t requested_buffer_size, std::error_code &ec) {
		if (reserve(std::max(requested_buffer_size, strlength), ec), ec) {
			return;
		}
		assert(_capacity >= strlength + 1);
		memcpy(_data, str, strlength);
		_size = strlength;
		// plant a sentinel at the end.
		write_buffer_edge_sentinel();
	}

}

