
/*

- keep only comments and function/class prototypes, i.e. only lines which start at left edge.
- keep left-edge C multi-line comments.

- 'trim' sequences of empty lines.

- Ditch code scopes: `{ ... }`
- Discard preprocessor statements that are not `#define`.
- Discard `extern "C" {` and anonymous `namespace` statements.

- Ditch remaining empty lines.

*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <span>
#include <chrono>
#include <algorithm>
#include <future>
#include <atomic>
#include <thread>
#include <format>

#ifdef CLI11_SINGLE_FILE
#include "CLI11.hpp"
#else
#include "CLI/CLI.hpp"
#endif
#include "CLI/Timer.hpp"

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include <absl/strings/internal/utf8.h>

#define GHC_USE_STD_FS

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// UTF8-Everywhere is the original behaviour of ghc::filesystem. But since v1.5 the Windows
// version defaults to std::wstring storage backend. Still all std::string will be interpreted
// as UTF-8 encoded. With this define you can enforce the old behavior on Windows, using
// std::string as backend and for fs::path::native() and char for fs::path::c_str(). This
// needs more conversions, so it is (and was before v1.5) slower, bot might help keeping source
// homogeneous in a multi-platform project.
// 
// #define GHC_WIN_DISABLE_WSTRING_STORAGE_TYPE
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Raise errors/exceptions when invalid unicode codepoints or UTF-8 sequences are found,
// instead of replacing them with the unicode replacement character (U+FFFD).
#define GHC_RAISE_UNICODE_ERRORS
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Automatic prefix windows path with "\\?\" if they would break the MAX_PATH length.
// instead of replacing them with the unicode replacement character (U+FFFD).
#define GHC_WIN_DISABLE_AUTO_PREFIXES
//#define GHC_WIN_AUTO_PREFIX_LONG_PATH
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include <ghc/fs_std.hpp>  // namespace fs = std::filesystem;   or   namespace fs = ghc::filesystem;

// WARNING/NOTE: `std::byte` instead of `unsigned char` results in all sorts of nasty compiler errors and type conversion warnings, so we ditched that type all around, regrettably.

typedef unsigned char   byte;


// run an async timer task which sets a mark every time a bit of progress MAY be shown.
// This takes care of the variable and sometimes obnoxiously high '.' dot progress rate/output.
class ProgressTimer
{
	std::future<int> t;
	std::atomic<bool> ticked = false;
	std::atomic<bool> must_stop = false;

public:
	void init(void)
	{
		t = std::async(std::launch::async, &ProgressTimer::timer_task, this);
	}

	~ProgressTimer()
	{
		must_stop = true;
		t.wait();
		(void)t.get();
	}

	int timer_task(void)
	{
		using namespace std::chrono_literals;

		ticked = true;

		while (!must_stop) {
			ticked = true;

			std::this_thread::sleep_for(125ms);
		}

		ticked = true;

		return 0;
	}

	void show_progress(void)
	{
		auto triggered = ticked.exchange(false);
		if (triggered) {
			std::cerr << ".";
		}
	}

	bool tick(void)
	{
		return ticked;
	}
};


// We accept '.' and '-' as file names representing stdin/stdout:
static bool is_stdin_stdout(const std::string &filename) {
	return (filename == "." || filename == "-" || filename == "/dev/stdin" || filename == "/dev/stdout");
}


static constexpr size_t operator""_MB(size_t cnt)
{
	return cnt * 0124 * 1024;
}

static constexpr size_t TAIL_SIZE = 8;

static void zero_the_tail(byte *ptr) {
	memset(ptr, 0, TAIL_SIZE);
}

/*
Rough content cleaner/stripper:

- keep only comments and function/class prototypes, i.e. only lines which start at left edge.
- keep left-edge C multi-line comments.

- 'trim' sequences of empty lines.

- Ditch code scopes: `{ ... }`
- Discard preprocessor statements that are not `#define`.
- Discard `extern "C" {` and anonymous `namespace` statements.

- Ditch remaining empty lines.

*/

static bool is_ascii_byte(byte c)
{
	return (c >= 32 && c < 127) || c == '\n' || c == '\r' || c == '\t';
}


// For Unicode code points 0 through 0x10FFFF, EncodeUTF8Char writes
// out the UTF-8 encoding into buffer, and returns the number of chars
// it wrote.
//
// As described in https://tools.ietf.org/html/rfc3629#section-3 , the encodings
// are:
//    00 -     7F : 0xxxxxxx
//    80 -    7FF : 110xxxxx 10xxxxxx
//   800 -   FFFF : 1110xxxx 10xxxxxx 10xxxxxx
// 10000 - 10FFFF : 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
//
// Values greater than 0x10FFFF are not supported and may or may not write
// characters into buffer, however never will more than kMaxEncodedUTF8Size
// bytes be written, regardless of the value of utf8_char.
enum { kMaxEncodedUTF8Size = 4 };
static size_t EncodeUTF8Char(char* buffer, char32_t utf8_char);

struct ShiftState {
	bool saw_high_surrogate = false;
	unsigned char bits = 0;
};

// Converts `wc` from UTF-16 or UTF-32 to UTF-8 and writes to `buf`. `buf` is
// assumed to have enough space for the output. `s` is used to carry state
// between successive calls with a UTF-16 surrogate pair. Returns the number of
// chars written, or `static_cast<size_t>(-1)` on failure.
//
// This is basically std::wcrtomb(), but always outputting UTF-8 instead of
// respecting the current locale.
static size_t WideToUtf8(wchar_t wc, char* buf, ShiftState& s);



static size_t EncodeUTF8Char(char* buffer, char32_t utf8_char) {
	if (utf8_char <= 0x7F) {
		*buffer = static_cast<char>(utf8_char);
		return 1;
	} else if (utf8_char <= 0x7FF) {
		buffer[1] = static_cast<char>(0x80 | (utf8_char & 0x3F));
		utf8_char >>= 6;
		buffer[0] = static_cast<char>(0xC0 | utf8_char);
		return 2;
	} else if (utf8_char <= 0xFFFF) {
		buffer[2] = static_cast<char>(0x80 | (utf8_char & 0x3F));
		utf8_char >>= 6;
		buffer[1] = static_cast<char>(0x80 | (utf8_char & 0x3F));
		utf8_char >>= 6;
		buffer[0] = static_cast<char>(0xE0 | utf8_char);
		return 3;
	} else {
		buffer[3] = static_cast<char>(0x80 | (utf8_char & 0x3F));
		utf8_char >>= 6;
		buffer[2] = static_cast<char>(0x80 | (utf8_char & 0x3F));
		utf8_char >>= 6;
		buffer[1] = static_cast<char>(0x80 | (utf8_char & 0x3F));
		utf8_char >>= 6;
		buffer[0] = static_cast<char>(0xF0 | utf8_char);
		return 4;
	}
}

static size_t WideToUtf8(wchar_t wc, char* buf, ShiftState& s) {
	// Reinterpret the output buffer `buf` as `unsigned char*` for subsequent
	// bitwise operations. This ensures well-defined behavior for bit
	// manipulations (avoiding issues with signed `char`) and is safe under C++
	// aliasing rules, as `unsigned char` can alias any type.
	auto* ubuf = reinterpret_cast<unsigned char*>(buf);
	const uint32_t v = static_cast<uint32_t>(wc);
	constexpr size_t kError = static_cast<size_t>(-1);

	if (v <= 0x007F) {
		// 1-byte sequence (U+0000 to U+007F).
		// 0xxxxxxx.
		ubuf[0] = (0b0111'1111 & v);
		s = {};  // Reset surrogate state.
		return 1;
	} else if (0x0080 <= v && v <= 0x07FF) {
		// 2-byte sequence (U+0080 to U+07FF).
		// 110xxxxx 10xxxxxx.
		ubuf[0] = 0b1100'0000 | (0b0001'1111 & (v >> 6));
		ubuf[1] = 0b1000'0000 | (0b0011'1111 & v);
		s = {};  // Reset surrogate state.
		return 2;
	} else if ((0x0800 <= v && v <= 0xD7FF) || (0xE000 <= v && v <= 0xFFFF)) {
		// 3-byte sequence (U+0800 to U+D7FF or U+E000 to U+FFFF).
		// Excludes surrogate code points U+D800-U+DFFF.
		// 1110xxxx 10xxxxxx 10xxxxxx.
		ubuf[0] = 0b1110'0000 | (0b0000'1111 & (v >> 12));
		ubuf[1] = 0b1000'0000 | (0b0011'1111 & (v >> 6));
		ubuf[2] = 0b1000'0000 | (0b0011'1111 & v);
		s = {};  // Reset surrogate state.
		return 3;
	} else if (0xD800 <= v && v <= 0xDBFF) {
		// High Surrogate (U+D800 to U+DBFF).
		// This part forms the first two bytes of an eventual 4-byte UTF-8 sequence.
		const unsigned char high_bits_val = (0b0000'1111 & (v >> 6)) + 1;

		// First byte of the 4-byte UTF-8 sequence (11110xxx).
		ubuf[0] = 0b1111'0000 | (0b0000'0111 & (high_bits_val >> 2));
		// Second byte of the 4-byte UTF-8 sequence (10xxxxxx).
		ubuf[1] = 0b1000'0000 |                           //
			(0b0011'0000 & (high_bits_val << 4)) |  //
			(0b0000'1111 & (v >> 2));
		// Set state for high surrogate after writing to buffer.
		s = {true, static_cast<unsigned char>(0b0000'0011 & v)};
		return 2;  // Wrote 2 bytes, expecting 2 more from a low surrogate.
	} else if (0xDC00 <= v && v <= 0xDFFF) {
		// Low Surrogate (U+DC00 to U+DFFF).
		// This part forms the last two bytes of a 4-byte UTF-8 sequence,
		// using state from a preceding high surrogate.
		if (!s.saw_high_surrogate) {
			// Error: Isolated low surrogate without a preceding high surrogate.
			// s remains in its current (problematic) state.
			// Caller should handle error.
			return kError;
		}

		// Third byte of the 4-byte UTF-8 sequence (10xxxxxx).
		ubuf[0] = 0b1000'0000 |                    //
			(0b0011'0000 & (s.bits << 4)) |  //
			(0b0000'1111 & (v >> 6));
		// Fourth byte of the 4-byte UTF-8 sequence (10xxxxxx).
		ubuf[1] = 0b1000'0000 | (0b0011'1111 & v);

		s = {};    // Reset surrogate state, pair complete.
		return 2;  // Wrote 2 more bytes, completing the 4-byte sequence.
	} else if constexpr (0xFFFF < std::numeric_limits<wchar_t>::max()) {
		// Conditionally compile the 4-byte direct conversion branch.
		// This block is compiled only if wchar_t can represent values > 0xFFFF.
		// It's placed after surrogate checks to ensure surrogates are handled by
		// their specific logic. This inner 'if' is the runtime check for the 4-byte
		// range. At this point, v is known not to be in the 1, 2, or 3-byte BMP
		// ranges, nor is it a surrogate code point.
		if (0x10000 <= v && v <= 0x10FFFF) {
			// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx.
			ubuf[0] = 0b1111'0000 | (0b0000'0111 & (v >> 18));
			ubuf[1] = 0b1000'0000 | (0b0011'1111 & (v >> 12));
			ubuf[2] = 0b1000'0000 | (0b0011'1111 & (v >> 6));
			ubuf[3] = 0b1000'0000 | (0b0011'1111 & v);
			s = {};  // Reset surrogate state.
			return 4;
		}
	}

	// Invalid wchar_t value (e.g., out of Unicode range, or unhandled after all
	// checks).
	s = {};  // Reset surrogate state.
	return kError;
}





// NOTE: this next part is a near-duplicate of the code in thirdparty/mujs/utf.c

typedef signed int Rune;	/* Code-point values in Unicode 4.0 are 21 bits wide.*/

enum
{
	UTFmax	= 4,		/* maximum bytes per rune */
	Runesync	= 0x80,		/* cannot represent part of a UTF sequence (<) */
	Runeself	= 0x80,		/* rune and UTF sequences are the same (<) */
	Runeerror	= 0xFFFD,	/* decoding error in UTF */
	Runemax	= 0x10FFFF,	/* maximum rune value */
};

enum
{
	Bit1 = 7,
	Bitx = 6,
	Bit2 = 5,
	Bit3 = 4,
	Bit4 = 3,
	Bit5 = 2,

	T1 = ((1<<(Bit1+1))-1) ^ 0xFF, /* 0000 0000 */
	Tx = ((1<<(Bitx+1))-1) ^ 0xFF, /* 1000 0000 */
	T2 = ((1<<(Bit2+1))-1) ^ 0xFF, /* 1100 0000 */
	T3 = ((1<<(Bit3+1))-1) ^ 0xFF, /* 1110 0000 */
	T4 = ((1<<(Bit4+1))-1) ^ 0xFF, /* 1111 0000 */
	T5 = ((1<<(Bit5+1))-1) ^ 0xFF, /* 1111 1000 */

	Rune1 = (1<<(Bit1+0*Bitx))-1, /* 0000 0000 0111 1111 */
	Rune2 = (1<<(Bit2+1*Bitx))-1, /* 0000 0111 1111 1111 */
	Rune3 = (1<<(Bit3+2*Bitx))-1, /* 1111 1111 1111 1111 */
	Rune4 = (1<<(Bit4+3*Bitx))-1, /* 0001 1111 1111 1111 1111 1111 */

	Maskx = (1<<Bitx)-1,	/* 0011 1111 */
	Testx = Maskx ^ 0xFF,	/* 1100 0000 */

	Bad = Runeerror,
};

static int
fz_chartorune(uint32_t *rune, const unsigned char *str, size_t n)
{
	uint32_t c, c1, c2, c3;
	uint32_t l;

	/*
	 * one character sequence
	 *	00000-0007F => T1
	 */
	if (n < 1)
		goto bad;
	c = *str;
	if (c < Tx) {
		*rune = c;
		return 1;
	}

	/*
	 * two character sequence
	 *	0080-07FF => T2 Tx
	 */
	if (n < 2)
		goto bad;
	c1 = *(str+1) ^ Tx;
	if (c1 & Testx)
		goto bad;
	if (c < T3) {
		if (c < T2)
			goto bad;
		l = ((c << Bitx) | c1) & Rune2;
		if (l <= Rune1)
			goto bad;
		*rune = l;
		return 2;
	}

	/*
	 * three character sequence
	 *	0800-FFFF => T3 Tx Tx
	 */
	if (n < 3)
		goto bad;
	c2 = *(str+2) ^ Tx;
	if (c2 & Testx)
		goto bad;
	if (c < T4) {
		l = ((((c << Bitx) | c1) << Bitx) | c2) & Rune3;
		if (l <= Rune2)
			goto bad;
		*rune = l;
		return 3;
	}

	/*
	 * four character sequence (21-bit value)
	 *	10000-1FFFFF => T4 Tx Tx Tx
	 */
	if (n < 4)
		goto bad;
	c3 = *(str+3) ^ Tx;
	if (c3 & Testx)
		goto bad;
	if (c < T5) {
		l = ((((((c << Bitx) | c1) << Bitx) | c2) << Bitx) | c3) & Rune4;
		if (l <= Rune3)
			goto bad;
		*rune = l;
		return 4;
	}
	/*
	 * Support for 5-byte or longer UTF-8 would go here, but
	 * since we don't have that, we'll just fall through to bad.
	 */

	 /*
	  * bad decoding
	  */
bad:
	*rune = Bad;
	return -1;
}

static int
fz_runetochar(unsigned char *str, uint32_t rune)
{
	/* Runes are unsigned, useful for range check. */
	unsigned int c = (unsigned int)rune;

	/*
	 * one character sequence
	 *	00000-0007F => 00-7F
	 */
	if (c <= Rune1) {
		str[0] = c;
		return 1;
	}

	/*
	 * two character sequence
	 *	0080-07FF => T2 Tx
	 */
	if (c <= Rune2) {
		str[0] = T2 | (c >> 1*Bitx);
		str[1] = Tx | (c & Maskx);
		return 2;
	}

	/*
	 * If the Rune is out of range, convert it to the error rune.
	 * Do this test here because the error rune encodes to three bytes.
	 * Doing it earlier would duplicate work, since an out of range
	 * Rune wouldn't have fit in one or two bytes.
	 */
	if (c > Runemax)
		c = Runeerror;

	/*
	 * three character sequence
	 *	0800-FFFF => T3 Tx Tx
	 */
	if (c <= Rune3) {
		if ((0x0800 <= c && c <= 0xD7FF) || (0xE000 <= c && c <= 0xFFFF)) {
			// valid code point
		} else {
			// surrogate ==> flag as error!
			c = Runeerror;
		}
		// 3-byte sequence (U+0800 to U+D7FF or U+E000 to U+FFFF).
		// Excludes surrogate code points U+D800-U+DFFF.
		str[0] = T3 | (c >> 2*Bitx);
		str[1] = Tx | ((c >> 1*Bitx) & Maskx);
		str[2] = Tx | (c & Maskx);
		return 3;
	}

	/*
	 * four character sequence (21-bit value)
	 *	10000-1FFFFF => T4 Tx Tx Tx
	 */
	str[0] = T4 | (c >> 3*Bitx);
	str[1] = Tx | ((c >> 2*Bitx) & Maskx);
	str[2] = Tx | ((c >> 1*Bitx) & Maskx);
	str[3] = Tx | (c & Maskx);
	return 4;
}


//
// Given that we always have TAIL_SIZE nil bytes following the tail end of the input buffer, it is safe to look ahead TAIL_SIZE or
// (if looking ahead further) until the next NUL byte.
//
// It is also safe to rewrite/destroy the input buffer in the space we will process.
//

static std::tuple<size_t, std::wstring> detect_and_convert_UCS2_UTF16_LE(const byte *src, size_t srcsize)
{
	// heuristic: expect at least TWO ASCII characters as leading part of any UTC2/UTF16 sequence:
	if (src[1] == 0 && src[3] == 0 && is_ascii_byte(src[0]) && is_ascii_byte(src[2])) {
		// treat as UTC2/UTF16 until we hit a snag ~ error.

		// convert UCS2/UTF16 LE to UTF8:
		std::wstring str;
		size_t i = 0;
		while (i + 1 < srcsize) {
			byte c1 = src[i++];
			byte c2 = src[i++];
			if (c1 == 0 && c2 == 0) {
				// end of string
				break;
			}
			if (c1 < 128 && c2 == 0) {
				// ASCII character
				str.push_back(c1);
			} else {
				// convert to code point:
				uint32_t cp = c2;
				cp = (cp << 8) | c1;

				// is this a legal Unicode codepoint or a UTF16 surrogate?
				// 
				// surrogate code points U+D800-U+DFFF.
				if (cp >= 0xD800 && cp < 0xE000) {
					// this must be a high surrogate and the next one we expect must be a low surrogate!

					// High Surrogate (U+D800 to U+DBFF).
					if (cp >= 0xD800 && cp < 0xDC00) {
						// This part forms the MSBits of the code point.

						c1 = src[i++];
						c2 = src[i++];
						// convert to code point, expect a low surrogate:
						uint32_t cp2 = c2;
						cp2 = (cp2 << 8) | c1;

						if (0xDC00 <= cp2 && cp2 <= 0xDFFF) {
							// Low Surrogate (U+DC00 to U+DFFF).
							// This part forms the LSBits of the code point.

							// combine the surrogates into the final code point:
							// https://en.wikipedia.org/wiki/UTF-16
							cp2 -= 0xDC00;
							cp -= 0xD800;
							cp <<= 10;
							cp |= cp2;
							cp += 0x10000;
							if (cp > Runemax) {
								// error, rewind
								i -= 4;
								break;
							}
							str.push_back(cp);
						}
						else {
							// error, rewind
							i -= 4;
							break;
						}
					} else {
						// unexpected low surrogate ==> error, rewind
						i -= 2;
						break;
					}
				} else {
					// UTF16 code point: 0x0080..0xFFFF
					str.push_back(cp);
				}
			}
		}
		// now scan through a possible all-NULs tail:
 		while (i < srcsize + TAIL_SIZE && src[i] == 0) {
			// NUL --> LF
			str.push_back('\n');
			i++;
		}

		return {i, str};
	}
	// else: not an expected heuristics match, hence reject/ignore:
	return {0, {}};
}

static size_t encode_wstring_as_UTF8(byte *dst, size_t dstsize, const std::wstring &str)
{
	// convert `str` back to UTF8:
	// we know that `str` only carries legal Unicode codepoints, so we should be good, as long
	// as the conversion fits into available space, followed by a fresh '\n' newline.
	//
	// As we MAY produce an UTF8 sequence that's *longer* than the original UTF16 sequence, we
	// MUST do a 'dry run' first to see if the UTF8 output will fit... If not, we need to bug out hard...
	// This is an artifact/bug/feature of our approach where we 'rewrite the data buffer' instead of
	// copying all content across into another destination/target space. Alas.
	//
	size_t i = 0;
	unsigned char scratch[4];
	for (auto cp : str) {
		int len = fz_runetochar(scratch, cp);
		if (i + len < dstsize) {
			i += len;
		} else {
			// whoops! overrun alarm!
			//
			// rewind the entire thing...
			return 0;
		}
	}
	// now redo the UTF8 encode for real: it won't overrun!
	i = 0;
	for (auto cp : str) {
		int len = fz_runetochar(&dst[i], cp);
		i += len;
	}
	return i;
}

static std::tuple<size_t, size_t> process_raw_data_into_text(byte *src, size_t srcsize, bool forced_process_all = false) {
	int prev_newline_count = 0;
	size_t j = 0;
	size_t i = 0;
	while (i < srcsize) {
		byte c = src[i++];
		if (c < 32) {
			switch (c) {
			default:
				// convert all (undesirable) control characters to LF:
				c = '\n';
				break;

			case 0: // --> detect UTC2/UTF16 input and decode accordingly...
				// TODO: detect UTF16/UCS BigEndian vs. Little Endian

				if (i > 1) {
					i--;
					auto [len, wstr] = detect_and_convert_UCS2_UTF16_LE(src + i, srcsize - i);
					if (len == 0) {
						// error condition: treat as sentinel NUL ==> NL
						i++;
						c = '\n';
						break;
					}
					// convert `wstr` to UTF8!
					i++;
					i += len;

					size_t n = encode_wstring_as_UTF8(&src[j], i - j /* available rewrite space */, wstr);
					if (n == 0) {
						// error condition: treat as sentinel NUL ==> NL
						i -= len;
						c = '\n';
						break;
					}
					continue;
				}

				// treat as sentinel NUL ==> NL
				c = '\n';
				break;

			case '\r':
				c = '\n';
				break;
			case '\n':
				break;
			case '\t':
				// c = ' ';
				break;
			}
		} else if (c < 127) {
			// ASCII: keep as is
		} else if (c == 127) {
			// DEL --> LF
			c = '\n';
		} else {
			// c > 127: assume UTF8; it's NOT ASCII, anyway!

			// UTF8 input expected/assumed.
			uint32_t rune = 0;
			i--;
			// thanks to our tail (TAIL_SIZE we don't have to worry about underruns.
			int l = fz_chartorune(&rune, src + i, srcsize - i);
			if (l < 1) {
				// error => discard (non-ASCII, non-UTF8).
				i++;
				c = '\n';
				break;
			}
			// else: copy the UTF8 byte seq:
			if (j != i) {
				memmove(src + j, src + i, l);
			}
			// else: no need to move as &src[j] == &src[i]
			j += l;
			i += l;
			continue;
		}
		src[j++] = c;
	}
	// add sentinel at the end:
	src[j] = 0;

	return {i, j};
}


static size_t fread_with_process(FILE *fin, ProgressTimer *progress, byte *buf, size_t bufsize)
{
	byte *baseptr = buf;
	byte *ptr = baseptr;
	byte *endptr = baseptr + bufsize;
	size_t spot_bufsize = (progress != nullptr ? 1024 : bufsize);
	size_t spot_count = 1;

	// load small-ish chunks of file content while keeping an eye on our progress ticker.
	// Once we have a rough estimate how many chunks we could fetch between progress 'ticks',
	// we update (increase) the chunk size to optimize reading from the file.
	//
	// This aims for a middle ground between maximum read buffering/speed and timely/smooth progress visual feedback.
	while (endptr > ptr) {
		size_t remainder = endptr - ptr;
		if (remainder > spot_bufsize)
			remainder = spot_bufsize;
		auto len = fread(ptr, 1, remainder, fin);
		ptr += len;
		if (ferror(fin))
			break;

		// show progress if requested
		if (progress) {
			if (!progress->tick()) {
				spot_count++;
			} else {
				progress->show_progress();
				spot_bufsize *= spot_count;
				spot_count = 1;
			}
		}

		if (feof(fin))
			break;
	}
	return ptr - baseptr;
}

/*
* Fetch input from stdin until EOF.
*
* Buffer everything and separate the input into text lines, which will be sorted and de-duplicated.
*/

int main(int argc, const char **argv) {
	// https://stackoverflow.com/questions/7587595/read-binary-data-from-stdcin
#if defined(_WIN32)
	freopen(NULL, "rb", stdin);
	_setmode(_fileno(stdin), _O_BINARY);
#endif

	CLI::App app{"buffered_tee"};
	CLI::Timer timer;
	ProgressTimer progress;

	std::vector<std::string> inFiles;
	app.add_option("--infile,-i", inFiles, "specify the file location of an input file") /* ->required() */;
	std::vector<std::string> outFiles;
	app.add_option("--outfile,-o", outFiles, "specify the file location of an output file") /* ->required() */;
	bool show_progress = false;
	app.add_flag("-p,--progress", show_progress);
	bool append_to_file = false;
	app.add_flag("-a,--append", append_to_file);
	bool quiet_mode = false;
	app.add_flag("-q,--quiet", quiet_mode);
	bool cleanup_stderr = false;
	app.add_flag("-c,--cleanup", cleanup_stderr, "replace all non-ASCII, non-printable characters in stderr log/progress output with '.'");
	std::optional<std::uint64_t> redux_opt;
	app.add_option("-r,--redux", redux_opt, "reduced stdout output / stderr progress noise: output 1 line for each N input lines.");

	CLI11_PARSE(app, argc, argv);

	if (outFiles.empty()) {
		std::cerr << "Warning: no output files specified. All you'll see is the progress/echo to stderr/stdout!\n    Use --outfile or -o to specify at least one output file." << std::endl;

		outFiles.push_back("-"); // use stdout as output
	}

	if (inFiles.empty()) {
		std::cerr << "Notice: no input files specified: STDIN is used instead!\n    Use --infile or -i to specify at least one output file." << std::endl;

		inFiles.push_back("-"); // use stdin as input
	}

	if (quiet_mode && redux_opt) {
		std::cerr << "Warning: --redux option is ignored when --quiet is enabled." << std::endl;
	}

	uint64_t redux_lines = redux_opt.value_or(0);

	progress.init();

	// read lines from stdin/inputs:
	// 
	// https://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf
	std::vector<std::string> lines;


	if (!quiet_mode) {
		if (show_progress) {
			std::cerr << "Reading from input files...";
		}
	}

	double timer_rd_time;
	std::optional<double> timer_wr_time;

	CLI::Timer timer_rd;
	{
		//CLI::Timer timer_rd;

		// See also: https://www.coniferproductions.com/posts/2022/10/25/reading-binary-files-cpp/
		// for some info about file I/O C++ madness.
		//
		// Here we use a different idiom to make it all happen while expecting arbitrary binary input files:
		// we deal with the Windows CR/LF vs. UNIX LF-only line ending issues ourselves, as part and
		// consequence of that 'we expect to deal with arbitrary binary files' stance.

		for (const auto &inFile : inFiles) {
			if (is_stdin_stdout(inFile)) {
				// use stdin

				//ifs.emplace_back(std::cin.rdbuf());

				// Create a unique_ptr to an uninitialized array of 16 MB
				const size_t bufsize = 16_MB;
				std::unique_ptr<byte[]> buf = std::make_unique_for_overwrite<byte[]>(bufsize + TAIL_SIZE);
				byte *baseptr = buf.get();
				byte *ptr = baseptr;
				byte *endptr = baseptr + bufsize;

				for (;;) {
					auto len = fread_with_process(stdin, (!quiet_mode && show_progress ? &progress : nullptr), ptr, endptr - ptr);
					if (ferror(stdin))
						break;
					ptr += len;

					zero_the_tail(ptr);

					auto [srclen, dstlen ] = process_raw_data_into_text(baseptr, ptr - baseptr);
					ptr += srclen;

					// shift remainder to start of buffer, if there's anything still pending:
					len = endptr - ptr;
					if (len > 0) {
						memmove(baseptr, ptr, len);
					}
					ptr = baseptr + len;
				}
				// and finally: deal with the last dregs
				{
					auto len = ptr - baseptr;
					if (len > 0) {
						zero_the_tail(ptr);

						auto [srclen, dstlen] = process_raw_data_into_text(baseptr, len, true);
					}
				}
			} else {
				// open file
				//
				// https://stackoverflow.com/questions/2409504/using-c-filestreams-fstream-how-can-you-determine-the-size-of-a-file
				// https://stackoverflow.com/questions/22984956/tellg-function-give-wrong-size-of-file/22986486#22986486
				std::ifstream ifs(inFile, std::ios::binary | std::ios::ate);
				if (!ifs) {
					std::cerr << std::endl << "Error opening input file: " << inFile << std::endl;
					return 1;
				}
				size_t filesize = fs::file_size(inFile);

				std::unique_ptr<byte[]> buf = std::make_unique_for_overwrite<byte[]>(filesize + TAIL_SIZE);
				byte *baseptr = buf.get();

				auto len = fread_with_process(stdin, (!quiet_mode && show_progress ? &progress : nullptr), baseptr, filesize);
				if (ferror(stdin))
					break;

				zero_the_tail(baseptr);

				auto [srclen, dstlen] = process_raw_data_into_text(baseptr, len, true);
			}
		}

#if 0
		// do NOT stop & get the elpased time WITHIN the scope block as that would introduce
		// the COSTLY mistake of NOT INCLUDING THE TIME SPENT in the destructors!!
		timer_rd_time = timer_rd();
#endif
	}
	timer_rd_time = timer_rd();

	// NOTE: right now, all input files have been read and *closed*; their content resides in lines[].

	size_t written_line_count = 0;

	if (lines.empty()) {
		if (!quiet_mode) {
			if (show_progress) {
				std::cerr << std::endl << "Warning: Input feed is empty (no text lines read). We will SKIP writing the output files!" << std::endl;
			}
		}
	} else {
		if (!quiet_mode) {
			if (show_progress) {
				std::cerr << std::endl;
			}
		}

		// write to output files
		//
		// Note: when any of them fails to open, then abort all output.
		CLI::Timer timer_wr;
		{
			bool stdout_is_one_of_the_outputs = false;
			std::vector<std::ofstream> ofs;
			for (const auto &outFile : outFiles) {
				if (is_stdin_stdout(outFile)) {
					// use stdout
					stdout_is_one_of_the_outputs = true;
				} else {
					std::ofstream of(outFile, (append_to_file ? std::ios::app : std::ios::trunc));
					if (!of) {
						std::cerr << "Error opening output file: " << outFile << std::endl;
						return 1;
					}
					ofs.push_back(std::move(of));
				}
			}

			if (!quiet_mode) {
				if (show_progress) {
					std::cerr << "Writing to output files...";
				}
			}

			for (/* const */ std::string& l : lines) {
				for (auto &outFile : ofs) {
					outFile << l << '\n';
				}
				if (stdout_is_one_of_the_outputs) {
					std::cout << l << '\n';
				}
				written_line_count++;

				// show progress if requested
				if (!quiet_mode) {
					if (redux_lines <= 1 || written_line_count % redux_lines == 1) {
						if (show_progress) {
							progress.show_progress();
						} else /* if (!stdout_is_one_of_the_outputs) */ {
							if (cleanup_stderr) {
								// replace all non-ASCII, non-printable characters in string with '.':
								std::replace_if(l.begin(), l.end(), [](char ch) {
									return (static_cast<unsigned char>(ch) < 32 || static_cast<unsigned char>(ch) > 126);
								}, '.');
							}
							std::cerr << l << '\n';
						}
					}
				}
			}

			//timer_wr_time = timer_wr();
		} // end of scope for the output files --> auto-close!

		timer_wr_time = timer_wr();

		if (!quiet_mode) {
			if (show_progress) {
				std::cerr << std::endl;
			}
		}
	}

	if (!quiet_mode) {
		std::cerr << "All done." << std::endl;
		std::cerr << written_line_count << " lines written." << std::endl;
		std::cerr << "Timing report:" << std::endl;

		auto cvt = [](double time) -> std::pair<double, std::string> {
			if (time < .000001)
				return {time * 1000000000, "ns"};
			if (time < .001)
				return {time * 1000000, "us"};
			if (time < 1)
				return {time * 1000, "ms"};
			return {time, "s"};
			};

		auto t = cvt(timer_rd_time);
		std::cerr << std::format("{:<25}{:9.3f}{:>3}", "Reading + processing inputs", t.first, t.second) << std::endl;
		t = cvt(timer_wr_time.value_or(0.0));
		std::cerr << std::format("{:<25}{:9.3f}{:>3}", "Writing to output", t.first, t.second) << std::endl;
		t = cvt(timer());
		std::cerr << std::format("{:<25}{:9.3f}{:>3}", "Total time taken", t.first, t.second) << std::endl;
	}

	return 0;
}

