
#include "Base.hpp"

#include <cstdlib>

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
		_size(strlen(str)),
		_capacity(_size + sentinel_size),
		_data(alloc_and_copy_string(str, _size, _capacity)) {
	}

	TextBuffer::TextBuffer(const char *str, size_t length, size_t requested_buffer_size) :
		_capacity(std::max(length + sentinel_size, requested_buffer_size)),
		_size(length),
		_data(alloc_and_copy_string(str, _size, _capacity)) {
	}

	TextBuffer::TextBuffer(const std::string_view &str, size_t requested_buffer_size) :
		_size(str.length()),
		_capacity(std::max(_size + sentinel_size, requested_buffer_size)),
		_data(alloc_and_copy_string(str.data(), _size, _capacity)) {
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
		std::cout << "move failed!\n";

		_data = reinterpret_cast<char *>(malloc(src._capacity));
		if (_data == nullptr)
			throw std::bad_alloc();
		_size = src._size;
		_capacity = src._capacity;
		memcpy(_data, src._data, src._size);
	}

	TextBuffer::TextBuffer(TextBuffer &&lvsrc):
		_size(std::move(lvsrc._size)),
		_data(std::move(lvsrc._data)),
		_capacity(std::move(lvsrc._capacity)) {
	}

	TextBuffer& TextBuffer::operator=(const TextBuffer& src)
	{
		std::cout << "copy assigned\n";

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
		memset(_data + _size, '\0', sentinel_size);

		return *this;
	}

	TextBuffer& TextBuffer::operator=(TextBuffer&& src)
	{
		std::cout << "move assigned\n";

		_size = std::move(src._size);
		_data = std::move(src._data);
		_capacity = std::move(src._capacity);

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

	void TextBuffer::CopyAndPrepare(const char *str, size_t strlength, size_t requested_buffer_size, std::error_code &ec) {
		if (reserve(std::max(requested_buffer_size, strlength), ec), ec) {
			return;
		}
		assert(_capacity >= strlength + 1);
		memcpy(_data, str, strlength);
		// plant a sentinel at the end.
		_data[strlength] = '\0';
	}

}

