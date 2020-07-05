/*
Simple string and Unicode library. Choice of public domain or MIT-0. See license statements at the end of this file.

David Reid - davidreidsoftware@gmail.com
*/

/*
Introduction
============
libcstr is a single file library for simple string and Unicode operations. To use it, do the following in one .c file:

    ```c
    #define LIBCSTR_IMPLEMENTATION
    #include "libcstr.h
    ```

The library is made up of three main parts:

    1) Custom implementations of some standard and some compiler-specific string APIs. Only a small number of these functions have been implemented, but more
       will be added as the need arises. These are useful to avoid certain compiler-specific quirks. For example, using `strcpy()` can be safe in the right
       situations, however modern versions of MSVC will always throw a warning regardless. In this case, it may be convenient to use a custom implementation
       to silence the warning. In addition, some safe versions (`_s()`) may not be available on all compilers, so libcstr implements some of these as well.

         * strlen()    -> utf8_strlen()
         * strcpy()    -> utf8_strcpy()
         * strcpy_s()  -> utf8_strcpy_s()
         * strncpy_s() -> utf8_strncpy_s()
         * strcat_s()  -> utf8_strcat_s()
         * strncat_s() -> utf8_strncat_s()
         * atoi_s()    -> utf8_atoi_s()

    2) Dynamically allocated strings via the `cstr` type. These are null terminated and compatible with `char*` and `const char*` strings, but also include
       some prefixed data containing the length of the string and the capacity of the internal buffer. See the documentation in the Dynamic Strings section
       below for details on this API.

    3) Unicode conversion APIs for converting between UTF-8, UTF-16 and UTF-32 encodings. These APIs should be flexible enough for the vast majority of use
       cases. See the documentation in the Unicode section below for details on this API.
       
At the time of writing, this library is more focused on simplicity of implementation than efficiency. If you have specific performance requirements you may
want to look at alternatives. Also, I'm only adding functionality as I need it so many things are likely missing.

If you need more UTF-8 manipulation functionality, consider using utf8.h here: https://github.com/sheredom/utf8.h. This is single file, public domain and
focused on UTF-8 with a much more complete library of functionality than what you'll find here.

This library has no external dependencies and is public domain. If your region does not recognize public domain, you can alternatively use MIT-0. See the
bottom of this file for license text.
*/

#ifndef libcstr_h
#define libcstr_h

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
    #define CSTR_INLINE __forceinline
#elif defined(__GNUC__)
    /*
    I've had a bug report where GCC is emitting warnings about functions possibly not being inlineable. This warning happens when
    the __attribute__((always_inline)) attribute is defined without an "inline" statement. I think therefore there must be some
    case where "__inline__" is not always defined, thus the compiler emitting these warnings. When using -std=c89 or -ansi on the
    command line, we cannot use the "inline" keyword and instead need to use "__inline__". In an attempt to work around this issue
    I am using "__inline__" only when we're compiling in strict ANSI mode.
    */
    #if defined(__STRICT_ANSI__)
        #define CSTR_INLINE __inline__ __attribute__((always_inline))
    #else
        #define CSTR_INLINE inline __attribute__((always_inline))
    #endif
#else
    #define CSTR_INLINE
#endif

#if !defined(CSTR_API)
    #if defined(CSTR_DLL)
        #if defined(_WIN32)
            #define CSTR_DLL_IMPORT  __declspec(dllimport)
            #define CSTR_DLL_EXPORT  __declspec(dllexport)
            #define CSTR_DLL_PRIVATE static
        #else
            #if defined(__GNUC__) && __GNUC__ >= 4
                #define CSTR_DLL_IMPORT  __attribute__((visibility("default")))
                #define CSTR_DLL_EXPORT  __attribute__((visibility("default")))
                #define CSTR_DLL_PRIVATE __attribute__((visibility("hidden")))
            #else
                #define CSTR_DLL_IMPORT
                #define CSTR_DLL_EXPORT
                #define CSTR_DLL_PRIVATE static
            #endif
        #endif

        #if defined(MINIAUDIO_IMPLEMENTATION) || defined(CSTR_IMPLEMENTATION)
            #define CSTR_API  CSTR_DLL_EXPORT
        #else
            #define CSTR_API  CSTR_DLL_IMPORT
        #endif
        #define CSTR_PRIVATE CSTR_DLL_PRIVATE
    #else
        #define CSTR_API     extern
        #define CSTR_PRIVATE static
    #endif
#endif

#include <stddef.h> /* For size_t. */

/* Sized types. Prefer built-in types. Fall back to stdint. */
#ifdef _MSC_VER
    #if defined(__clang__)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wlanguage-extension-token"
        #pragma GCC diagnostic ignored "-Wlong-long"
        #pragma GCC diagnostic ignored "-Wc++11-long-long"
    #endif
    typedef   signed __int8  cstr_int8;
    typedef unsigned __int8  cstr_uint8;
    typedef   signed __int16 cstr_int16;
    typedef unsigned __int16 cstr_uint16;
    typedef   signed __int32 cstr_int32;
    typedef unsigned __int32 cstr_uint32;
    typedef   signed __int64 cstr_int64;
    typedef unsigned __int64 cstr_uint64;
    #if defined(__clang__)
        #pragma GCC diagnostic pop
    #endif
#else
    #define CSTR_HAS_STDINT
    #include <stdint.h>
    typedef int8_t   cstr_int8;
    typedef uint8_t  cstr_uint8;
    typedef int16_t  cstr_int16;
    typedef uint16_t cstr_uint16;
    typedef int32_t  cstr_int32;
    typedef uint32_t cstr_uint32;
    typedef int64_t  cstr_int64;
    typedef uint64_t cstr_uint64;
#endif
typedef cstr_uint32   cstr_bool32;
#define CSTR_TRUE     1
#define CSTR_FALSE    0

#if defined(_MSC_VER) && !defined(_WCHAR_T_DEFINED)
typedef cstr_uint16 wchar_t;
#endif

typedef char        cstr_utf8;
typedef cstr_uint16 cstr_utf16;
typedef cstr_uint32 cstr_utf32;


/* Define NULL for some compilers. */
#ifndef NULL
#define NULL 0
#endif

#include <stdarg.h> /* For va_list */
#include <errno.h>  /* For errno_t */

#define cstr_npos ((size_t)-1)

CSTR_API size_t utf8_strlen(const cstr_utf8* src);      /* Returns the number of bytes, *not* the number of the Unicode code points. */
CSTR_API cstr_utf8* utf8_strcpy(cstr_utf8* dst, const cstr_utf8* src);
CSTR_API int utf8_strcpy_s(cstr_utf8* dst, size_t dstCap, const cstr_utf8* src);
CSTR_API int utf8_strncpy_s(cstr_utf8* dst, size_t dstCap, const cstr_utf8* src, size_t count);
CSTR_API int utf8_strcat_s(cstr_utf8* dst, size_t dstCap, const cstr_utf8* src);
CSTR_API int utf8_strncat_s(cstr_utf8* dst, size_t dstCap, const cstr_utf8* src, size_t count);
CSTR_API int utf8_itoa_s(int value, cstr_utf8* dst, size_t dstCap, int radix);
/*
CSTR_API int utf8_snprintf(cstr_utf8* dst, size_t dstCap, const cstr_utf8* fmt, ...);
CSTR_API int utf8_vsnprintf(cstr_utf8* dst, size_t dstCap, const cstr_utf8* fmt, va_list args);
*/

CSTR_API size_t utf16_strlen(const cstr_utf16* src);    /* Returns the number of shorts, *not* the number of the Unicode code points. */
CSTR_API size_t utf32_strlen(const cstr_utf32* src);    /* Returns the number of ints, *not* the number of the Unicode code points. */


/**************************************************************************************************************************************************************

Dynamic Strings
===============
This is a simple API for working with dynamically allocated strings. It only supports basic operations and will be expanded over time as required. The idea is
that the memory for the string is allocated on the heap with CSTR_MALLOC/CSTR_REALLOC/CSTR_FREE, with the length and capacity stored in the same allocation
just before the raw string data. The `cstr` type is an alias for `char*` and is null terminated which means it can be used anywhere where a `char*` parameter
is used.

Below is the memory layout of a `cstr` string.

    ```
                                  +---------------------------------------------------------------------------------------------------+
                                  | `cstr_cap()`                                                                                      |
                                  +------------------------------------------------------------------------------+                    |
                                  | `cstr_len()`                                                                 |                    |
    +--------------+--------------+------------------------------------------------------------------------------+--------------------+-----------------+
    | Cap (size_t) | Len (size_t) | .............................. String Content .............................. | Extra Space ('\0') | Null Terminator |         
    +--------------+--------------+------------------------------------------------------------------------------+--------------------+-----------------+
    ^                             ^ <-- `cstr` starts here                                                                                              ^
    +-----------------------------+---------------------------------------------------------------------------------------------------------------------+
            Prefixed Data                                                 C Style, Null Terminated String Data
    ```

The allocation of extra space is mainly just to optimize memory allocations and avoid excessive resizing of the internal buffer. Note that the first byte of
the extra space is always '\0', thus null terminating the string content. The illustration above includes explicit mention of the null terminator to show that
it is _not_ included in the value returned by `cstr_cap()` nor `cstr_len()` and to also show that the null terminator is always explicitly included as it's
possible (and likely) that the extra space can be zero bytes in length.

Whenever the string is modified, a copy of the new string is returned. Below is a usage example:

    ```c
    cstr myString;
    myString = cstr_new("My new string");                               // Create the dynamically allocated string.
    myString = cstr_cat(myString, " and some concatenated content");    // Append something to the string.
    myString = cstr_set(myString, "The Replacement String);             // Replace the string with new content.

    char buffer[256];
    strcpy(buffer, myString);   // Use the `cstr` type just like any other `char*` or `const char*` string.

    cstr_free(myString);        // Always free the string when you're finished with it.
    ```


API Reference
-------------
cstr cstr_alloc(size_t len)
    Allocates memory for the new string, setting the capacity to `len`, the length to 0 and the content of the string to zero. Returns NULL if out of memory.
    The returned object must be freed with `cstr_free()` after use. You can use this with `cstr_setn()` to output the result of an arbitrary string operation
    to the memory owned by a `cstr`. To do this like the following:

        cstr str = cstr_alloc(strLen);      // Allocate memory
        strcpy_s(str, strLen+1 otherStr);   // Perform a string operation
        cstr_setn(str, str, strLen);        // Set the length of the string by calling cstr_setn() on the same string, passing in an explicit length.

void cstr_free(cstr str)
    Frees a `cstr` string.

cstr cstr_newn(const char* pOther, size_t otherLen)
    Creates a new string, initialized with the content of another string of a specified length. Returns NULL if out of memory or `pOther` is NULL.

cstr cstr_new(const char* pOther)
    Creates a new string, initialized with the content of another null terminated string. The null terminator of the other string is used to determine the end
    of the string. Returns NULL if out of memory or `pOther` is NULL.

cstr cstr_newv(const char* pFormat, va_list args)
    Creates a new string using printf() style formatting via a pre-initialized `va_list` object. Returns NULL if out of memory or `pFormat` is NULL or a
    formatting error occurs.

cstr cstr_newf(const char* pFormat, ...)
    Same a `cstr_newv()` only formats the string from a dynamic variable list.

cstr cstr_setn(cstr str, const char* pOther, size_t otherLen)
    Changes the content of the given string to that of another string of the specified length. Returns NULL if out of memory. Will be set to an empty string if
    `pOther` is NULL.

cstr cstr_set(cstr str, const char* pOther)
    Changes the content of the given string to that of another null terminated string. The null terminator of the other string is used to determine the end of
    the string. Returns NULL if out of memory. Will bet set to an empty string if `pOther` is NULL.

cstr cstr_catn(cstr str, const char* pOther, size_t otherLen)
    Appends a string based on the specified number of characters. Returns NULL if out of memory. Returns `str` unmodified if `pOther` is NULL.

cstr cstr_cat(cstr str, const char* pOther)
    Appends a null terminated string, with the null terminated used to determine the end of the string. Returns NULL if out of memory. Returns `str` unmodified
    if `pOther` is NULL.

size_t cstr_len(cstr str)
    Returns the length of the string in `char`s. This does _not_ return the number of Unicode code points. It is analogous to `strlen()`, only it retrieves the
    length from a variable rather than calculating it on the fly. Returns 0 if `str` is NULL.

size_t cstr_cap(cstr str)
    Returns the capacity of the string in `char`s. This does not include the null terminator or the bytes required for the length and capacity pre-fixed data.
    Returns 0 if `str` is NULL.

size_t cstr_find(const char* str, const char* other)
    Finds a string within another string, returning it's offset. If the string cannot be found, cstr_npos is returned.

const char* cstr_substr_tagged(const char* str, const char* pTagBeg, const char* pTagEnd, size_t* pLen)
    Finds a substring containing only the section inside, and including, the specified tags.

cstr cstr_new_substr_tagged(const char* str, const char* pTagBeg, const char* pTagEnd)
    Constructs a new string based on the substring returned by cstr_substr_tagged().

cstr cstr_replace_range(cstr str, size_t replaceOffset, size_t replaceLen, const char* pOther, size_t otherLen)
    Replaces a section of the specified string with a sub-string of another. Returns NULL if an error occurrs, otherwise the returned string should replace the
    input string.

cstr cstr_replace_range_tagged(cstr8 str, const char* pTagBeg, const char* pTagEnd, const char* pOther, const char* pOtherTagBeg, const char* pOtherTagEnd, cstr_bool32 keepTagsOnSeparateLines)
    Replaces a section of the specified string based on a begin and end tag, for both the input and other strings. The replacement string can optionally be
    wrapped around new line characters with keepTagsOnSeparateLines set to true. This will not replace the tags, and will include the tags of the other string.
    Returns NULL if an error occurs, otherwise the returned string should replace the input string.

**************************************************************************************************************************************************************/
typedef cstr_utf8*  cstr8;
typedef cstr_utf16* cstr16;
typedef cstr_utf32* cstr32;
typedef wchar_t*    cstrw;
typedef cstr8       cstr;

#ifndef CSTR_NO_UTF8
CSTR_API cstr8 cstr8_alloc(size_t len);
CSTR_API void cstr8_free(cstr8 str);
CSTR_API cstr8 cstr8_newn(const char* pOther, size_t otherLen);
CSTR_API cstr8 cstr8_new(const char* pOther);
CSTR_API cstr8 cstr8_newv(const char* pFormat, va_list args);
CSTR_API cstr8 cstr8_newf(const char* pFormat, ...);
CSTR_API cstr8 cstr8_setn(cstr8 str, const char* pOther, size_t otherLen);
CSTR_API cstr8 cstr8_set(cstr8 str, const char* pOther);
CSTR_API cstr8 cstr8_catn(cstr8 str, const char* pOther, size_t otherLen);
CSTR_API cstr8 cstr8_cat(cstr8 str, const char* pOther);
CSTR_API size_t cstr8_len(cstr8 str);
CSTR_API size_t cstr8_cap(cstr8 str);
CSTR_API size_t cstr8_find(const char* str, const char* other);  /* Returns cstr_npos if string not found, otherwise returns offset in bytes. */
CSTR_API size_t cstr8_findn(const char* str, size_t strLen, const char* other, size_t otherLen);
CSTR_API size_t cstr8_find_last(const char* str, const char* other);
CSTR_API size_t cstr8_findn_last(const char* str, size_t strLen, const char* other, size_t otherLen);
CSTR_API const char* cstr8_substr_tagged(const char* str, const char* pTagBeg, const char* pTagEnd, size_t* pLen);
CSTR_API cstr8 cstr8_new_substr_tagged(const char* str, const char* pTagBeg, const char* pTagEnd);
CSTR_API cstr8 cstr8_replace_range(cstr8 str, size_t replaceOffset, size_t replaceLen, const char* pOther, size_t otherLen);
CSTR_API cstr8 cstr8_replace_range_tagged(cstr8 str, const char* pTagBeg, const char* pTagEnd, const char* pOther, const char* pOtherTagBeg, const char* pOtherTagEnd, cstr_bool32 keepTagsOnSeparateLines);
CSTR_API cstr8 cstr8_new_trim(const char* pOther);
CSTR_API cstr8 cstr8_newn_trim(const char* pOther, size_t otherLen);
CSTR_API cstr8 cstr8_remove_at(cstr8 str, size_t index);

#define cstr_alloc                  cstr8_alloc
#define cstr_free                   cstr8_free
#define cstr_newn                   cstr8_newn
#define cstr_new                    cstr8_new
#define cstr_newv                   cstr8_newv
#define cstr_newf                   cstr8_newf
#define cstr_setn                   cstr8_setn
#define cstr_set                    cstr8_set
#define cstr_catn                   cstr8_catn
#define cstr_cat                    cstr8_cat
#define cstr_len                    cstr8_len
#define cstr_cap                    cstr8_cap
#define cstr_find                   cstr8_find
#define cstr_findn                  cstr8_findn
#define cstr_find_last              cstr8_find_last
#define cstr_findn_last             cstr8_findn_last
#define cstr_substr_tagged          cstr8_substr_tagged
#define cstr_new_substr_tagged      cstr8_new_substr_tagged
#define cstr_replace_range          cstr8_replace_range
#define cstr_replace_range_tagged   cstr8_replace_range_tagged
#define cstr_new_trim               cstr8_new_trim;
#define cstr_newn_trim              cstr8_newn_trim;
#define cstr_remove_at              cstr8_remove_at;
#endif


/**************************************************************************************************************************************************************

Unicode
=======
The Unicode API is mainly intended for converting between UTF-8, UTF-16 and UTF-32 encoded strings. The API currently only supports conversion, BOM detection
and endian swapping.

The conversion API is flexible and should support the vast majority of use cases. It supports both null terminated and sized strings (sub-strings), with
support for outputting the number of basic code units that were generated on output and consumed on input. It also supports handling of byte-order-marks (BOM)
in addition to detection of invalid code points with control over whether or not they should abort with an error, or be replaced with a replacement code point.

Each of the conversion APIs also support the retrieval of the length of the output string by passing in NULL for the output buffer. This works in the same
manner as `snprintf()`. In addition to this, explicit APIs exist for retrieving the output length for those who prefer the explicitness.

Support is also included for both little- and big-endian layouts for UTF-16 and UTF-32. These APIs will be named appropriate, such as the following:

    cstr_utf16le_to_utf32le()

In the function above, the input data is assumed to be little-endian for both the UTF-16 and UTF-32 data. If the endian is not specified, it will use the BOM,
if any, to determine the endianness. If the BOM is not present it assumes native endian:

    cstr_utf16_to_utf32()

Note that you cannot convert from one encoding to another encoding of different endianness. If you need to do this, first convert the data based on the input
endianness, and then perform a post-processing step on the output data with `cstr_swap_endian_*()`, like the following:

    cstr_utf16be_to_utf32be(...);   // Convert using big-endian layout.
    cstr_swap_endian_utf32(...);    // Convert the output buffer from big-endian to little-endian.

In the example above we converted UTF-16BE data to UTF-32BE data, but wanted the final output to be UTF-32LE. We did this by simply performing an endian-swap
as a post-processing operation. To swap the endianness of a UTF-16 string, use `cstr_swap_endian_utf16()` instead.


API Reference
-------------
Note that [encoding-type] refers to one of the following basic types:
    
    cstr_utf8  (char)
    cstr_utf16 (cstr_uint16)
    cstr_utf32 (cstr_uint32)

The names of parameters in this documentation is different to the actual declarations, just to make it easier when referring back to them in documentation. In
the actual declarations, the names of parameters include the type for clarity, such as `pUTF8` or `utf16Len`, etc.

Each of the conversion routines accepts a set of flags as their last parmaeter. These flags can be combined using the bitwise OR operator. Below are the
available flags and their meaning:

    CSTR_FORBID_BOM
        If set, return an error of a BOM is found at the start of the string.

    CSTR_ERROR_ON_INVALID_CODE_POINT
        If set, return an error if an invalid code point is encounted. If this is unset the invalid code point will be replaced wht the replacement which is
        defined by CSTR_UNICODE_REPLACEMENT_CODE_POINT.

Errors are returned via an errno_t code. This can be any of the standard result tokens that appear on almost all platforms, such as `ENOMEM` and `EINVAL`. In
addition to these codes, the following custom codes may also be returned:

    CSTR_SUCCESS (0)
        Returned when the operation is successful. This is always equal to 0.

    CSTR_EBOM
        Returned when the CSTR_FORBID_BOM flag is set and a BOM is found at the start of the string.

    CSTR_ECODEPOINT
        Returned when the CSTR_ERROR_ON_INVALID_CODE_POINT flag is set and an invalid code point is encounted.


errno_t cstr_*_to_*([encoding-type]* pOutput, size_t outputCap, size_t* pOutputLen, const [encoding-type]* pInput, size_t inputLen, size_t* pInputLenProcessed, cstr_uint32 flags)

    cstr_utf8_to_utf16ne
    cstr_utf8_to_utf16le
    cstr_utf8_to_utf16be
    cstr_utf8_to_utf16
    cstr_utf8_to_utf32ne
    cstr_utf8_to_utf32le
    cstr_utf8_to_utf32be
    cstr_utf8_to_utf32

    cstr_utf16ne_to_utf8
    cstr_utf16le_to_utf8
    cstr_utf16be_to_utf8
    cstr_utf16_to_utf8
    cstr_utf16ne_to_utf32ne
    cstr_utf16le_to_utf32le
    cstr_utf16be_to_utf32be
    cstr_utf16_to_utf32

    cstr_utf32ne_to_utf8
    cstr_utf32le_to_utf8
    cstr_utf32be_to_utf8
    cstr_utf32_to_utf8
    cstr_utf32ne_to_utf16ne
    cstr_utf32le_to_utf16le
    cstr_utf32be_to_utf16be
    cstr_utf32_to_utf16

    Converts from one encoding to another. The output buffer can be NULL in which case it will only calculate the required size of the output buffer, not
    including the null terminator. The `outputCap` parameter specifies the size of the output buffer, including the null terminator. `pOutputLen` is optional,
    but if specified will receive the length of the output string, not including the null terminator.

    It is invalid for the input string (`pInput`) to be NULL. This will result in `EINVAL` being returned. The input length (`inputLen`) can be `(size_t)-1` in
    which case it will be treated as null terminated. Note that if the input length is specified as something other than `(size_t)-1`, no null-teminator
    detection logic will be used when converting the string. This means processing will continue even if a null terminator is encounted in the input string.

    Processing continues until either the output buffer has been completely filled (determined by `outputCap`) or the end of the input string has been
    exhausted (determined by `inputLen`).
    
    If all you want to do is validate the input string, set the appropriate flags (last parameter - `flags`) and set both the output buffer (`pOutput`) and the
    output length (`pOutputLen`) to NULL. This will result in no output data and no output length, but will still return a value indicating whether or not the
    input string is valid. In this case you'll usually want to set the `CSTR_ERROR_ON_INVALID_CODE_POINT` flag. The `pInputLenProcessed` parameter can be used
    to determine the location of the error within the input string.

    If the `CSTR_ERROR_ON_INVALID_CODE_POINT` flag is set and the input string contains an invalid character, `CSTR_ECODEPOINT` will be returned. If the
    `CSTR_FORBID_BOM` flag is set and the string starts with a byte-order-mark, `CSTR_EBOM` will be returned. If an output buffer is specified and it's not
    large enough to contain the converted string, `ENOMEM` will be returned. For input strings in UTF-8 or UTF-16 format, if the input buffer is exhausted
    while in the middle of decoding a code point, `EINVAL` will be returned.  This will happen if, for example, the second half of a UTF-16 surrogate pair is
    required but cannot be accessed because the end of the input buffer has already been reached. Whenever an error occurs, `pInputLenProcessed` will be set to
    the end of the last good code point and start of the errneous code point. Use this to determine where you got up to in processing and the location of the
    erroneous code point.


errno_t cstr_*_to_*_len(size_t* pOutputLen, const [encoding-type]* pInput, size_t inputLen, size_t* pInputLenProcessed, cstr_uint32 flags)

    cstr_utf8_to_utf16_len
    cstr_utf8_to_utf16ne_len
    cstr_utf8_to_utf16le_len
    cstr_utf8_to_utf16be_len
    cstr_utf8_to_utf32_len
    cstr_utf8_to_utf32ne_len
    cstr_utf8_to_utf32le_len
    cstr_utf8_to_utf32be_len

    cstr_utf16ne_to_utf8_len
    cstr_utf16le_to_utf8_len
    cstr_utf16be_to_utf8_len
    cstr_utf16_to_utf8_len
    cstr_utf16ne_to_utf32_len
    cstr_utf16le_to_utf32_len
    cstr_utf16be_to_utf32_len
    cstr_utf16ne_to_utf32ne_len
    cstr_utf16le_to_utf32le_len
    cstr_utf16be_to_utf32be_len
    cstr_utf16_to_utf32_len

    cstr_utf32ne_to_utf8_len
    cstr_utf32le_to_utf8_len
    cstr_utf32be_to_utf8_len
    cstr_utf32_to_utf8_len
    cstr_utf32ne_to_utf16_len
    cstr_utf32le_to_utf16_len
    cstr_utf32be_to_utf16_len
    cstr_utf32ne_to_utf16ne_len
    cstr_utf32le_to_utf16le_len
    cstr_utf32be_to_utf16be_len
    cstr_utf32_to_utf16_len

    Retrieves the length of the output string after conversion, not including the null terminator, but does not perform any actual conversion. If an error
    occurs, a result code will be returned to indicate the nature of the error. The `pInput` and `inputLen` parameters have the same meaning as the base
    conversion routines.


void cstr_swap_endian_utf16(cstr_utf16* pUTF16, size_t count)
void cstr_swap_endian_utf32(cstr_utf32* pUTF32, size_t count)
    Performs an in-place endian swap on the given UTF-16 or UTF-32 string. If `count` is equal to `(size_t)-1`, it will assume the string is null terminated.


**************************************************************************************************************************************************************/
/*
Custom error codes. We return errno_t types from Unicode operations to detect invalid data, and sometimes it needs to be distinguished what went wrong. We're
using negative numbers, starting from bit 14 to try our best to avoid clashing. However, since we do not modify the global errno value and aren't calling any
system calls, it shouldn't actually matter if something else coincidentally uses the same code. So long as we don't clash with any of the common ones, which
should be unlikely as they all seems to be positive in all of the implementations I've seen, we should be OK.
*/
#define CSTR_SUCCESS    0       /* No error. */
#define CSTR_EBOM       -16384  /* Invalid BOM */
#define CSTR_ECODEPOINT -16385  /* Invalid code point. */

#define CSTR_UNICODE_MIN_CODE_POINT                         0x000000
#define CSTR_UNICODE_MAX_CODE_POINT                         0x10FFFF
#define CSTR_UNICODE_REPLACEMENT_CODE_POINT                 0x00FFFD
#define CSTR_UNICODE_REPLACEMENT_CODE_POINT_LENGTH_UTF8     3
#define CSTR_UNICODE_REPLACEMENT_CODE_POINT_LENGTH_UTF16    1
#define CSTR_UNICODE_REPLACEMENT_CODE_POINT_LENGTH_UTF32    1

#define CSTR_FORBID_BOM                                     (1 << 1)
#define CSTR_ERROR_ON_INVALID_CODE_POINT                    (1 << 2)

CSTR_API cstr_bool32 cstr_is_utf16_bom_le(const cstr_uint8 bom[2]);
CSTR_API cstr_bool32 cstr_is_utf16_bom_be(const cstr_uint8 bom[2]);
CSTR_API cstr_bool32 cstr_is_utf32_bom_le(const cstr_uint8 bom[4]);
CSTR_API cstr_bool32 cstr_is_utf32_bom_be(const cstr_uint8 bom[4]);

CSTR_API cstr_bool32 cstr_has_utf8_bom(const cstr_uint8* pBytes, size_t len);
CSTR_API cstr_bool32 cstr_has_utf16_bom(const cstr_uint8* pBytes, size_t len);
CSTR_API cstr_bool32 cstr_has_utf32_bom(const cstr_uint8* pBytes, size_t len);

CSTR_API void cstr_swap_endian_utf16(cstr_utf16* pUTF16, size_t count);
CSTR_API void cstr_swap_endian_utf32(cstr_utf32* pUTF32, size_t count);


/* UTF-8 */
CSTR_API errno_t cstr_utf8_to_utf16_len(size_t* pUTF16Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags);
static CSTR_INLINE errno_t cstr_utf8_to_utf16ne_len(size_t* pUTF16Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags) { return cstr_utf8_to_utf16_len(pUTF16Len, pUTF8, utf8Len, pUTF8LenProcessed, flags); }
static CSTR_INLINE errno_t cstr_utf8_to_utf16le_len(size_t* pUTF16Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags) { return cstr_utf8_to_utf16_len(pUTF16Len, pUTF8, utf8Len, pUTF8LenProcessed, flags); }
static CSTR_INLINE errno_t cstr_utf8_to_utf16be_len(size_t* pUTF16Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags) { return cstr_utf8_to_utf16_len(pUTF16Len, pUTF8, utf8Len, pUTF8LenProcessed, flags); }

CSTR_API errno_t cstr_utf8_to_utf16ne(cstr_utf16* pUTF16, size_t utf16Cap, size_t* pUTF16Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf8_to_utf16le(cstr_utf16* pUTF16, size_t utf16Cap, size_t* pUTF16Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf8_to_utf16be(cstr_utf16* pUTF16, size_t utf16Cap, size_t* pUTF16Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags);
static CSTR_INLINE errno_t cstr_utf8_to_utf16(cstr_utf16* pUTF16, size_t utf16Cap, size_t* pUTF16Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags) { return cstr_utf8_to_utf16ne(pUTF16, utf16Cap, pUTF16Len, pUTF8, utf8Len, pUTF8LenProcessed, flags); }

CSTR_API errno_t cstr_utf8_to_utf32_len(size_t* pUTF32Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags);
static CSTR_INLINE errno_t cstr_utf8_to_utf32ne_len(size_t* pUTF32Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags) { return cstr_utf8_to_utf32_len(pUTF32Len, pUTF8, utf8Len, pUTF8LenProcessed, flags); }
static CSTR_INLINE errno_t cstr_utf8_to_utf32le_len(size_t* pUTF32Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags) { return cstr_utf8_to_utf32_len(pUTF32Len, pUTF8, utf8Len, pUTF8LenProcessed, flags); }
static CSTR_INLINE errno_t cstr_utf8_to_utf32be_len(size_t* pUTF32Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags) { return cstr_utf8_to_utf32_len(pUTF32Len, pUTF8, utf8Len, pUTF8LenProcessed, flags); }

CSTR_API errno_t cstr_utf8_to_utf32ne(cstr_utf32* pUTF32, size_t utf32Cap, size_t* pUTF32Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf8_to_utf32le(cstr_utf32* pUTF32, size_t utf32Cap, size_t* pUTF32Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf8_to_utf32be(cstr_utf32* pUTF32, size_t utf32Cap, size_t* pUTF32Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags);
static CSTR_INLINE errno_t cstr_utf8_to_utf32(cstr_utf32* pUTF32, size_t utf32Cap, size_t* pUTF32Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags) { return cstr_utf8_to_utf32ne(pUTF32, utf32Cap, pUTF32Len, pUTF8, utf8Len, pUTF8LenProcessed, flags); }


/* UTF-16 */
CSTR_API errno_t cstr_utf16ne_to_utf8_len(size_t* pUTF8Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf16le_to_utf8_len(size_t* pUTF8Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf16be_to_utf8_len(size_t* pUTF8Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf16_to_utf8_len(size_t* pUTF8Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags);

CSTR_API errno_t cstr_utf16ne_to_utf8(cstr_utf8* pUTF8, size_t utf8Cap, size_t* pUTF8Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf16le_to_utf8(cstr_utf8* pUTF8, size_t utf8Cap, size_t* pUTF8Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf16be_to_utf8(cstr_utf8* pUTF8, size_t utf8Cap, size_t* pUTF8Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf16_to_utf8(cstr_utf8* pUTF8, size_t utf8Cap, size_t* pUTF8Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags);

CSTR_API errno_t cstr_utf16ne_to_utf32_len(size_t* pUTF32Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf16le_to_utf32_len(size_t* pUTF32Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf16be_to_utf32_len(size_t* pUTF32Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags);
static CSTR_INLINE errno_t cstr_utf16ne_to_utf32ne_len(size_t* pUTF32Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags) { return cstr_utf16ne_to_utf32_len(pUTF32Len, pUTF16, utf16Len, pUTF16LenProcessed, flags); }
static CSTR_INLINE errno_t cstr_utf16le_to_utf32le_len(size_t* pUTF32Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags) { return cstr_utf16le_to_utf32_len(pUTF32Len, pUTF16, utf16Len, pUTF16LenProcessed, flags); }
static CSTR_INLINE errno_t cstr_utf16be_to_utf32be_len(size_t* pUTF32Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags) { return cstr_utf16be_to_utf32_len(pUTF32Len, pUTF16, utf16Len, pUTF16LenProcessed, flags); }
CSTR_API errno_t cstr_utf16_to_utf32_len(size_t* pUTF32Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags);

CSTR_API errno_t cstr_utf16ne_to_utf32ne(cstr_utf32* pUTF32, size_t utf32Cap, size_t* pUTF32Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf16le_to_utf32le(cstr_utf32* pUTF32, size_t utf32Cap, size_t* pUTF32Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf16be_to_utf32be(cstr_utf32* pUTF32, size_t utf32Cap, size_t* pUTF32Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf16_to_utf32(cstr_utf32* pUTF32, size_t utf32Cap, size_t* pUTF32Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags);


/* UTF-32 */
CSTR_API errno_t cstr_utf32ne_to_utf8_len(size_t* pUTF8Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf32le_to_utf8_len(size_t* pUTF8Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf32be_to_utf8_len(size_t* pUTF8Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf32_to_utf8_len(size_t* pUTF8Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags);

CSTR_API errno_t cstr_utf32ne_to_utf8(cstr_utf8* pUTF8, size_t utf8Cap, size_t* pUTF8Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf32le_to_utf8(cstr_utf8* pUTF8, size_t utf8Cap, size_t* pUTF8Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf32be_to_utf8(cstr_utf8* pUTF8, size_t utf8Cap, size_t* pUTF8Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf32_to_utf8(cstr_utf8* pUTF8, size_t utf8Cap, size_t* pUTF8Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags);

CSTR_API errno_t cstr_utf32ne_to_utf16_len(size_t* pUTF16Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf32le_to_utf16_len(size_t* pUTF16Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf32be_to_utf16_len(size_t* pUTF16Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags);
static CSTR_INLINE errno_t cstr_utf32ne_to_utf16ne_len(size_t* pUTF16Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags) { return cstr_utf32ne_to_utf16_len(pUTF16Len, pUTF32, utf32Len, pUTF32LenProcessed, flags); }
static CSTR_INLINE errno_t cstr_utf32le_to_utf16le_len(size_t* pUTF16Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags) { return cstr_utf32le_to_utf16_len(pUTF16Len, pUTF32, utf32Len, pUTF32LenProcessed, flags); }
static CSTR_INLINE errno_t cstr_utf32be_to_utf16be_len(size_t* pUTF16Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags) { return cstr_utf32be_to_utf16_len(pUTF16Len, pUTF32, utf32Len, pUTF32LenProcessed, flags); }
CSTR_API errno_t cstr_utf32_to_utf16_len(size_t* pUTF16Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags);

CSTR_API errno_t cstr_utf32ne_to_utf16ne(cstr_utf16* pUTF16, size_t utf16Cap, size_t* pUTF16Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf32le_to_utf16le(cstr_utf16* pUTF16, size_t utf16Cap, size_t* pUTF16Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf32be_to_utf16be(cstr_utf16* pUTF16, size_t utf16Cap, size_t* pUTF16Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags);
CSTR_API errno_t cstr_utf32_to_utf16(cstr_utf16* pUTF16, size_t utf16Cap, size_t* pUTF16Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags);


/**************************************************************************************************************************************************************

Utilities

**************************************************************************************************************************************************************/
#define cstr_is_null_or_empty(str) ((str) == NULL || (str)[0] == 0)

/* UTF-32 */
CSTR_API cstr_bool32 cstr_utf32_is_null_or_whitespace(const cstr_utf32* pUTF32, size_t utf32Len);

/* UTF-16 */

/* UTF-8 */
CSTR_API cstr_bool32 cstr_utf8_is_null_or_whitespace(const cstr_utf8* pUTF8, size_t utf8Len);
CSTR_API size_t cstr_utf8_ltrim_offset(const cstr_utf8* pUTF8, size_t utf8Len);
CSTR_API size_t cstr_utf8_rtrim_offset(const cstr_utf8* pUTF8, size_t utf8Len);
CSTR_API size_t cstr_utf8_next_line(const cstr_utf8* pUTF8, size_t utf8Len, size_t* pThisLineLen);

/* Default Wrappers (UTF-8) */
static CSTR_INLINE cstr_bool32 cstr_is_null_or_whitespace(const cstr_utf8* pUTF8, size_t utf8Len) { return cstr_utf8_is_null_or_whitespace(pUTF8, utf8Len); }
static CSTR_INLINE size_t cstr_ltrim_offset(const cstr_utf8* pUTF8, size_t utf8Len) { return cstr_utf8_ltrim_offset(pUTF8, utf8Len); }
static CSTR_INLINE size_t cstr_rtrim_offset(const cstr_utf8* pUTF8, size_t utf8Len) { return cstr_utf8_rtrim_offset(pUTF8, utf8Len); }
static CSTR_INLINE size_t cstr_next_line(const cstr_utf8* pUTF8, size_t utf8Len, size_t* pThisLineLen) { return cstr_utf8_next_line(pUTF8, utf8Len, pThisLineLen); }


#ifdef __cplusplus
}
#endif
#endif  /* libcstr_h */


/************************************************************************************************************************************************************
*************************************************************************************************************************************************************

IMPLEMENTATION

*************************************************************************************************************************************************************
************************************************************************************************************************************************************/
#if defined(LIBCSTR_IMPLEMENTATION)
#ifndef libcstr_c
#define libcstr_h

/* CPU Architecture */
#if defined(__x86_64__) || defined(_M_X64)
    #define CSTR_X64
#elif defined(__i386) || defined(_M_IX86)
    #define CSTR_X86
#elif defined(__arm__) || defined(_M_ARM)
    #define CSTR_ARM
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1300
    #define CSTR_HAS_BYTESWAP16_INTRINSIC
    #define CSTR_HAS_BYTESWAP32_INTRINSIC
    #define CSTR_HAS_BYTESWAP64_INTRINSIC
#elif defined(__clang__)
    #if __has_builtin(__builtin_bswap16)
        #define CSTR_HAS_BYTESWAP16_INTRINSIC
    #endif
    #if __has_builtin(__builtin_bswap32)
        #define CSTR_HAS_BYTESWAP32_INTRINSIC
    #endif
    #if __has_builtin(__builtin_bswap64)
        #define CSTR_HAS_BYTESWAP64_INTRINSIC
    #endif
#elif defined(__GNUC__)
    #if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))
        #define CSTR_HAS_BYTESWAP32_INTRINSIC
        #define CSTR_HAS_BYTESWAP64_INTRINSIC
    #endif
    #if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
        #define CSTR_HAS_BYTESWAP16_INTRINSIC
    #endif
#endif

#include <stdio.h>  /* For sprintf() */

#if !defined(CSTR_MALLOC) || !defined(CSTR_CALLOC) || !defined(CSTR_REALLOC) || !defined(CSTR_FREE)
#include <stdlib.h> /* For malloc(), calloc(), realloc(), free() */
#endif
#ifndef CSTR_MALLOC
#define CSTR_MALLOC(sz) malloc((sz))
#endif
#ifndef CSTR_CALLOC
#define CSTR_CALLOC(sz) calloc((sz), 1)
#endif
#ifndef CSTR_REALLOC
#define CSTR_REALLOC(p, sz) realloc((p), (sz))
#endif
#ifndef CSTR_FREE
#define CSTR_FREE(p) free((p))
#endif

#ifndef CSTR_ASSERT
#include <assert.h>
#define CSTR_ASSERT(condition) assert(condition);
#endif

#ifndef CSTR_COPY_MEMORY
#include <string.h> /* For memcpy() */
#define CSTR_COPY_MEMORY(dst, src, sz)  memcpy((dst), (src), (sz))
#endif
#ifndef CSTR_MOVE_MEMORY
#include <string.h> /* For memmove() */
#define CSTR_MOVE_MEMORY(dst, src, sz)  memmove((dst), (src), (sz))
#endif
#ifndef CSTR_ZERO_MEMORY
#include <string.h> /* For memset() */
#define CSTR_ZERO_MEMORY(dst, sz)       memset((dst), 0, (sz))
#endif
#define CSTR_ZERO_OBJECT(dst)           CSTR_ZERO_MEMORY((dst), sizeof(*(dst)))

#define CSTR_COUNTOF(p)                 (sizeof(p) / sizeof((p)[0]))

#define CSTR_HEADER_SIZE_IN_BYTES       (sizeof(size_t) + sizeof(size_t))


static CSTR_INLINE cstr_bool32 cstr_is_little_endian()
{
#if defined(CSTR_X86) || defined(CSTR_X64)
    return CSTR_TRUE;
#elif defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && __BYTE_ORDER == __LITTLE_ENDIAN
    return CSTR_TRUE;
#else
    int n = 1;
    return (*(char*)&n) == 1;
#endif
}

static CSTR_INLINE cstr_bool32 cstr_is_big_endian()
{
    return !cstr_is_little_endian();
}

static CSTR_INLINE cstr_uint16 cstr_swap_endian_uint16(cstr_uint16 n)
{
#ifdef CSTR_HAS_BYTESWAP16_INTRINSIC
    #if defined(_MSC_VER)
        return _byteswap_ushort(n);
    #elif defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap16(n);
    #else
        #error "This compiler does not support the byte swap intrinsic."
    #endif
#else
    return ((n & 0xFF00) >> 8) |
           ((n & 0x00FF) << 8);
#endif
}

static CSTR_INLINE cstr_uint32 cstr_swap_endian_uint32(cstr_uint32 n)
{
#ifdef CSTR_HAS_BYTESWAP32_INTRINSIC
    #if defined(_MSC_VER)
        return _byteswap_ulong(n);
    #elif defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap32(n);
    #else
        #error "This compiler does not support the byte swap intrinsic."
    #endif
#else
    return ((n & 0xFF000000) >> 24) |
           ((n & 0x00FF0000) >>  8) |
           ((n & 0x0000FF00) <<  8) |
           ((n & 0x000000FF) << 24);
#endif
}


static CSTR_INLINE cstr_uint16 cstr_be2host_16(cstr_uint16 n)
{
    if (cstr_is_little_endian()) {
        return cstr_swap_endian_uint16(n);
    }

    return n;
}

static CSTR_INLINE cstr_uint32 cstr_be2host_32(cstr_uint32 n)
{
    if (cstr_is_little_endian()) {
        return cstr_swap_endian_uint32(n);
    }

    return n;
}

static CSTR_INLINE cstr_uint16 cstr_le2host_16(cstr_uint16 n)
{
    if (!cstr_is_little_endian()) {
        return cstr_swap_endian_uint16(n);
    }

    return n;
}

static CSTR_INLINE cstr_uint32 cstr_le2host_32(cstr_uint32 n)
{
    if (!cstr_is_little_endian()) {
        return cstr_swap_endian_uint32(n);
    }

    return n;
}


static CSTR_INLINE cstr_uint16 cstr_host2be_16(cstr_uint16 n)
{
    if (cstr_is_little_endian()) {
        return cstr_swap_endian_uint16(n);
    }

    return n;
}

static CSTR_INLINE cstr_uint32 cstr_host2be_32(cstr_uint32 n)
{
    if (cstr_is_little_endian()) {
        return cstr_swap_endian_uint32(n);
    }

    return n;
}

static CSTR_INLINE cstr_uint16 cstr_host2le_16(cstr_uint16 n)
{
    if (!cstr_is_little_endian()) {
        return cstr_swap_endian_uint16(n);
    }

    return n;
}

static CSTR_INLINE cstr_uint32 cstr_host2le_32(cstr_uint32 n)
{
    if (!cstr_is_little_endian()) {
        return cstr_swap_endian_uint32(n);
    }

    return n;
}


CSTR_API size_t utf8_strlen(const cstr_utf8* src)
{
    const cstr_utf8* end;

    CSTR_ASSERT(src != NULL);
    
    end = src;
    while (end[0] != '\0') {
        end += 1;
    }

    return end - src;
}

CSTR_API cstr_utf8* utf8_strcpy(cstr_utf8* dst, const cstr_utf8* src)
{
    char* dstorig;

    CSTR_ASSERT(dst != NULL);
    CSTR_ASSERT(src != NULL);

    dstorig = dst;

    /* No, we're not using this garbage: while (*dst++ = *src++); */
    for (;;) {
        *dst = *src;
        if (*src == '\0') {
            break;
        }

        dst += 1;
        src += 1;
    }

    return dstorig;
}

CSTR_API int utf8_strcpy_s(cstr_utf8* dst, size_t dstCap, const cstr_utf8* src)
{
    size_t i;

    if (dst == 0) {
        return EINVAL;
    }
    if (dstCap == 0) {
        return ERANGE;
    }
    if (src == 0) {
        dst[0] = '\0';
        return EINVAL;
    }

    for (i = 0; i < dstCap && src[i] != '\0'; ++i) {
        dst[i] = src[i];
    }

    if (i < dstCap) {
        dst[i] = '\0';
        return 0;
    }

    dst[0] = '\0';
    return ERANGE;
}

CSTR_API int utf8_strncpy_s(cstr_utf8* dst, size_t dstCap, const cstr_utf8* src, size_t count)
{
    size_t maxcount;
    size_t i;

    if (dst == 0) {
        return EINVAL;
    }
    if (dstCap == 0) {
        return EINVAL;
    }
    if (src == 0) {
        dst[0] = '\0';
        return EINVAL;
    }

    maxcount = count;
    if (count == ((size_t)-1) || count >= dstCap) {        /* -1 = _TRUNCATE */
        maxcount = dstCap - 1;
    }

    for (i = 0; i < maxcount && src[i] != '\0'; ++i) {
        dst[i] = src[i];
    }

    if (src[i] == '\0' || i == count || count == ((size_t)-1)) {
        dst[i] = '\0';
        return 0;
    }

    dst[0] = '\0';
    return ERANGE;
}

CSTR_API int utf8_strcat_s(cstr_utf8* dst, size_t dstCap, const cstr_utf8* src)
{
    char* dstorig;

    if (dst == 0) {
        return EINVAL;
    }
    if (dstCap == 0) {
        return ERANGE;
    }
    if (src == 0) {
        dst[0] = '\0';
        return EINVAL;
    }

    dstorig = dst;

    while (dstCap > 0 && dst[0] != '\0') {
        dst    += 1;
        dstCap -= 1;
    }

    if (dstCap == 0) {
        return EINVAL;  /* Unterminated. */
    }

    while (dstCap > 0 && src[0] != '\0') {
        *dst++ = *src++;
        dstCap -= 1;
    }

    if (dstCap > 0) {
        dst[0] = '\0';
    } else {
        dstorig[0] = '\0';
        return ERANGE;
    }

    return 0;
}

CSTR_API int utf8_strncat_s(cstr_utf8* dst, size_t dstCap, const cstr_utf8* src, size_t count)
{
    char* dstorig;

    if (dst == 0) {
        return EINVAL;
    }
    if (dstCap == 0) {
        return ERANGE;
    }
    if (src == 0) {
        return EINVAL;
    }

    dstorig = dst;

    while (dstCap > 0 && dst[0] != '\0') {
        dst    += 1;
        dstCap -= 1;
    }

    if (dstCap == 0) {
        return EINVAL;  /* Unterminated. */
    }

    if (count == ((size_t)-1)) {        /* _TRUNCATE */
        count = dstCap - 1;
    }

    while (dstCap > 0 && src[0] != '\0' && count > 0) {
        *dst++ = *src++;
        dstCap -= 1;
        count  -= 1;
    }

    if (dstCap > 0) {
        dst[0] = '\0';
    } else {
        dstorig[0] = '\0';
        return ERANGE;
    }

    return 0;
}

CSTR_API int utf8_itoa_s(int value, cstr_utf8* dst, size_t dstCap, int radix)
{
    int sign;
    unsigned int valueU;
    char* dstEnd;

    if (dst == NULL || dstCap == 0) {
        return EINVAL;
    }
    if (radix < 2 || radix > 36) {
        dst[0] = '\0';
        return EINVAL;
    }

    sign = (value < 0 && radix == 10) ? -1 : 1;     /* The negative sign is only used when the base is 10. */

    if (value < 0) {
        valueU = -value;
    } else {
        valueU = value;
    }

    dstEnd = dst;
    do
    {
        int remainder = valueU % radix;
        if (remainder > 9) {
            *dstEnd = (char)((remainder - 10) + 'a');
        } else {
            *dstEnd = (char)(remainder + '0');
        }

        dstEnd += 1;
        dstCap -= 1;
        valueU /= radix;
    } while (dstCap > 0 && valueU > 0);

    if (dstCap == 0) {
        dst[0] = '\0';
        return EINVAL;  /* Ran out of room in the output buffer. */
    }

    if (sign < 0) {
        *dstEnd++ = '-';
        dstCap -= 1;
    }

    if (dstCap == 0) {
        dst[0] = '\0';
        return EINVAL;  /* Ran out of room in the output buffer. */
    }

    *dstEnd = '\0';


    /* At this point the string will be reversed. */
    dstEnd -= 1;
    while (dst < dstEnd) {
        char temp = *dst;
        *dst = *dstEnd;
        *dstEnd = temp;

        dst += 1;
        dstEnd -= 1;
    }

    return 0;
}


CSTR_API size_t utf16_strlen(const cstr_utf16* src)
{
    const cstr_utf16* end;

    CSTR_ASSERT(src != NULL);
    
    end = src;
    while (end[0] != '\0') {
        end += 1;
    }

    return end - src;
}

CSTR_API size_t utf32_strlen(const cstr_utf32* src)
{
    const cstr_utf32* end;

    CSTR_ASSERT(src != NULL);
    
    end = src;
    while (end[0] != '\0') {
        end += 1;
    }

    return end - src;
}




#ifndef CSTR_NO_UTF8
int cstr8_vscprintf(const char* pFormat, va_list args)
{
#if defined(_MSC_VER)
    #if _MSC_VER > 1200
        return _vscprintf(pFormat, args);
    #else
        /*
        We need to emulate _vscprintf() for the VC6 build. This can be made more efficient, but since it's only VC6 I'm happy to keep this simple. In the VC6
        build we can implement this in terms of _vsnprintf().
        */
        int result;
        char* pTempBuffer = NULL;
        size_t tempBufferCap = 1024;

        if (pFormat == NULL) {
            errno = EINVAL;
            return -1;
        }

	    for (;;) {
            char* pNewTempBuffer = (char*)CSTR_REALLOC(pTempBuffer, tempBufferCap, NULL);    /* TODO: Add support for custom memory allocators? */
            if (pNewTempBuffer == NULL) {
                CSTR_FREE(pTempBuffer, NULL);
                errno = ENOMEM;
                return -1;  /* Out of memory. */
            }

            pTempBuffer = pNewTempBuffer;

            result = _vsnprintf(pTempBuffer, tempBufferCap, pFormat, args);
            CSTR_FREE(pTempBuffer, NULL);
        
            if (result != -1) {
                break;  /* Got it. */
            }

            /* Buffer wasn't big enough. Ideally it'd be nice to use an error code to know the reason for sure, but this is reliable enough. */
            tempBufferCap *= 2;
	    }

        return result;
    #endif
#else
    return vsnprintf(NULL, 0, pFormat, args);
#endif
}

static int cstr8_vsprintf(char* pOutput, const char* pFormat, va_list args)
{
    /* WARNING: This should only be used by first measuring the output string by setting pOutput to NULL. */

    if (pOutput == NULL) {
        return cstr8_vscprintf(pFormat, args);
    } else {
    #if (!defined(_MSC_VER) || _MSC_VER >= 1900) && !defined(__STRICT_ANSI__)
        return vsnprintf(pOutput, (size_t)-1, pFormat, args);   /* We're lying about the length here. We should only be calling this internally, and only when computing the length beforehand, so it should be safe. */
    #else
        return vsprintf(pOutput, pFormat, args);
    #endif
    }
}

static CSTR_INLINE size_t cstr8_allocation_size(size_t cap)
{
    return CSTR_HEADER_SIZE_IN_BYTES + cap + 1; /* +1 for null terminator. */
}

static CSTR_INLINE void* cstr8_to_allocation_address(cstr8 str)
{
    return str - CSTR_HEADER_SIZE_IN_BYTES;
}

static CSTR_INLINE cstr8 cstr8_from_allocation_address(void* pAllocationAddress)
{
    return (char*)pAllocationAddress + CSTR_HEADER_SIZE_IN_BYTES;
}

static CSTR_INLINE void cstr8_set_cap(cstr8 str, size_t cap)
{
    ((size_t*)cstr8_to_allocation_address(str))[0] = cap;
}

static CSTR_INLINE size_t cstr8_get_cap(cstr8 str)
{
    return ((size_t*)cstr8_to_allocation_address(str))[0];
}

static CSTR_INLINE void cstr8_set_len(cstr8 str, size_t len)
{
    ((size_t*)cstr8_to_allocation_address(str))[1] = len;
}

static CSTR_INLINE size_t cstr8_get_len(cstr8 str)
{
    return ((size_t*)cstr8_to_allocation_address(str))[1];
}

static cstr8 cstr8_realloc(cstr8 str, size_t cap)
{
    void* addr = CSTR_REALLOC(cstr8_to_allocation_address(str), cstr8_allocation_size(cap));
    if (addr == NULL) {
        return NULL;    /* Failed */
    }

    cstr8_set_cap(cstr8_from_allocation_address(addr), cap);

    return cstr8_from_allocation_address(addr);
}

CSTR_API cstr8 cstr8_alloc(size_t len)
{
    char* str;

    str = CSTR_CALLOC(cstr8_allocation_size(len));
    if (str == NULL) {
        return NULL;    /* Out of memory. */
    }

    return str + CSTR_HEADER_SIZE_IN_BYTES;
}

CSTR_API void cstr8_free(cstr8 str)
{
    if (str == NULL) {
        return;
    }

    CSTR_FREE(cstr8_to_allocation_address(str));
}

CSTR_API cstr8 cstr8_newn(const char* pOther, size_t otherLen)
{
    cstr8 str;

    if (pOther == NULL) {
        return NULL;
    }

    if (otherLen == (size_t)-1) {
        otherLen = utf8_strlen(pOther);
    }

    str = cstr8_alloc(otherLen);
    if (str == NULL) {
        return NULL;    /* Out of memory. */
    }

    utf8_strncpy_s(str, otherLen+1, pOther, otherLen);  /* We've already calculated the length. No need for the added overhead of using the _s() version. */

    cstr8_set_cap(str, otherLen);
    cstr8_set_len(str, otherLen);

    return str;
}

CSTR_API cstr8 cstr8_new(const char* pOther)
{
    if (pOther == NULL) {
        return NULL;
    }

    return cstr8_newn(pOther, utf8_strlen(pOther));
}

CSTR_API cstr8 cstr8_newv(const char* pFormat, va_list args)
{
    va_list args2;
    size_t  len;
    cstr8   str;

    if (pFormat == NULL) {
        return NULL;
    }

#if !defined(_MSC_VER) || _MSC_VER >= 1800
    va_copy(args2, args);
#else
    args2 = args;
#endif

    len = cstr8_vsprintf(NULL, pFormat, args2);
    if (len < 0) {
        return NULL;
    }

    str = cstr8_alloc(len);
    if (str == NULL) {
        return str; /* Out of memory. */
    }

    cstr8_vsprintf(str, pFormat, args);

    cstr8_set_cap(str, len);
    cstr8_set_len(str, len);

    va_end(args2);

    return str;
}

CSTR_API cstr cstr8_newf(const char* pFormat, ...)
{
    va_list args;
    cstr8 str;

    if (pFormat == NULL) {
        return NULL;
    }

    va_start(args, pFormat);
    str = cstr8_newv(pFormat, args);
    va_end(args);

    return str;
}

CSTR_API cstr8 cstr8_setn(cstr8 str, const char* pOther, size_t otherLen)
{
    if (pOther == NULL) {
        pOther   = "";
        otherLen = 0;
    }

    if (str == NULL) {
        return cstr8_newn(pOther, otherLen);
    }  else {
        if (otherLen == (size_t)-1) {
            otherLen = utf8_strlen(pOther);
        }

        if (str != pOther) {
            size_t cap = cstr8_get_cap(str);

            if (cap < otherLen) {
                cap = otherLen;
                str = cstr8_realloc(str, cap);
                if (str == NULL) {
                    return NULL;    /* Out of memory. Return NULL. The caller can worry about memory management if it's important to them. */
                }
            }

            CSTR_COPY_MEMORY(str, pOther, otherLen);
        } else {
            /* str and pOther are the same string. No need for a data copy, but we do need to set the length (calculated at the top if pOther is null terminated). */
        }
        
        str[otherLen] = '\0';
        cstr8_set_len(str, otherLen);

        return str;
    }
}

CSTR_API cstr8 cstr8_set(cstr8 str, const char* pOther)
{
    if (pOther == NULL) {
        pOther = "";
    }

    return cstr8_setn(str, pOther, utf8_strlen(pOther));
}


CSTR_API cstr8 cstr8_catn(cstr8 str, const char* pOther, size_t otherLen)
{
    if (pOther == NULL) {
        return str;
    }

    if (str == NULL) {
        return cstr8_newn(pOther, otherLen);
    } else {
        size_t cap = cstr8_get_cap(str);
        size_t len = cstr8_get_len(str);

        if (otherLen == (size_t)-1) {
            otherLen = utf8_strlen(pOther);
        }

        if (cap < len + otherLen) {
            cap = len + otherLen;
            str = cstr8_realloc(str, cap);
            if (str == NULL) {
                return NULL;
            }
        }

        CSTR_COPY_MEMORY(str + len, pOther, otherLen);
        str[len + otherLen] = '\0';

        cstr8_set_len(str, len + otherLen);

        return str;
    }
}

CSTR_API cstr8 cstr8_cat(cstr8 str, const char* pOther)
{
    if (pOther == NULL) {
        return str;
    }

    return cstr8_catn(str, pOther, utf8_strlen(pOther));
}



CSTR_API size_t cstr8_len(cstr8 str)
{
    if (str == NULL) {
        return 0;
    }

    return cstr8_get_len(str);
}

CSTR_API size_t cstr8_cap(cstr8 str)
{
    if (str == NULL) {
        return 0;
    }

    return cstr8_get_cap(str);
}


CSTR_API size_t cstr8_find(const char* str, const char* other)
{
    return cstr8_findn(str, (size_t)-1, other, (size_t)-1);
}

CSTR_API size_t cstr8_findn(const char* str, size_t strLen, const char* other, size_t otherLen)
{
    size_t strOff;

    if (str == NULL || other == NULL) {
        return cstr_npos;
    }

    if (strLen == (size_t)-1) {
        strLen = utf8_strlen(str);
    }

    if (otherLen == (size_t)-1) {
        otherLen = utf8_strlen(other);
    }

    if (strLen == 0 || otherLen == 0) {
        return cstr_npos;
    }

    strOff = 0;
    while ((strLen - strOff) >= otherLen) {
        cstr_bool32 found = CSTR_TRUE;
        size_t i;
        for (i = 0; i < otherLen; i += 1) {
            if (str[strOff + i] != other[i]) {
                found = CSTR_FALSE;
                break;
            }
        }

        if (found) {
            return strOff;
        }

        strOff += 1;
    }

    /* Getting here means we didn't find the other string at all. */
    return cstr_npos;
}

CSTR_API size_t cstr8_find_last(const char* str, const char* other)
{
    return cstr8_findn_last(str, (size_t)-1, other, (size_t)-1);
}

CSTR_API size_t cstr8_findn_last(const char* str, size_t strLen, const char* other, size_t otherLen)
{
    size_t last = cstr_npos;
    size_t runningOffset = 0;

    if (str == NULL || other == NULL) {
        return cstr_npos;
    }

    if (strLen == (size_t)-1) {
        strLen = utf8_strlen(str);
    }

    if (otherLen == (size_t)-1) {
        otherLen = utf8_strlen(other);
    }

    if (strLen == 0 || otherLen == 0) {
        return cstr_npos;
    }

    for (;;) {
        size_t next = cstr8_findn(str + runningOffset, strLen - runningOffset, other, otherLen);
        if (next == cstr_npos) {
            break;  /* Didn't find any new occurances. */
        }

        last = runningOffset + next;

        runningOffset += next + otherLen;
    }

    return last;
}


CSTR_API const char* cstr8_substr_tagged(const char* str, const char* pTagBeg, const char* pTagEnd, size_t* pLen)
{
    size_t offsetBeg;
    size_t offsetEnd;

    if (pLen != NULL) {
        *pLen = 0;
    }

    if (pTagBeg == NULL || pTagBeg[0] == '\0') {
        offsetBeg = 0;
    } else {
        offsetBeg = cstr8_find(str, pTagBeg);
        if (offsetBeg == cstr_npos) {
            return NULL; /* Could not find the begin tag in the other string. */
        }
    }

    if (pTagEnd == NULL || pTagEnd[0] == '\0') {
        offsetEnd = utf8_strlen(str);
    } else {
        offsetEnd = cstr8_find(str + offsetBeg + utf8_strlen(pTagBeg), pTagEnd);
        if (offsetEnd == cstr_npos) {
            return NULL; /* Could not find the end tag in the other string. */
        } else {
            offsetEnd += offsetBeg + utf8_strlen(pTagBeg) + utf8_strlen(pTagEnd);
        }
    }

    if (pLen != NULL) {
        *pLen = (offsetEnd - offsetBeg);
    }

    return str + offsetBeg;
}

CSTR_API cstr8 cstr8_new_substr_tagged(const char* str, const char* pTagBeg, const char* pTagEnd)
{
    size_t len;
    str = cstr8_substr_tagged(str, pTagBeg, pTagEnd, &len);
    if (str == NULL) {
        return NULL;
    }

    return cstr8_newn(str, len);
}

static cstr8 cstr8_replace_range_ex(cstr8 str, size_t replaceOffset, size_t replaceLen, const char* pOther, size_t otherLen, const char* pOtherPrepend, const char* pOtherAppend)
{
    cstr8 newStr;

    if (str == NULL || replaceOffset + replaceLen > cstr8_len(str)) {
        return str;
    }

    if (replaceLen == 0) {
        return str; /* Nothing to replace. */
    }

    /* We can allow pOther to be NULL in which case it can be the same as a remove. */
    if (pOther == NULL) {
        pOther = "";
    }

    if (otherLen == (size_t)-1) {
        otherLen = utf8_strlen(pOther);
    }

    
    /* The string is split into 3 sections: the part before the replace, the replacement itself, and the part after the replacement. */

    /* Pre-replacement. */
    newStr = cstr8_newn(str, replaceOffset);

    /* Replacement. */
    if (pOtherPrepend != NULL) { newStr = cstr8_cat(newStr, pOtherPrepend); }
    newStr = cstr8_catn(newStr, pOther, otherLen);
    if (pOtherAppend  != NULL) { newStr = cstr8_cat(newStr, pOtherAppend ); }

    /* Post-replacement. */
    newStr = cstr8_catn(newStr, str + (replaceOffset + replaceLen), cstr8_len(str) - (replaceOffset + replaceLen));

    if (newStr == NULL) {
        return NULL;
    }

    /* We're done. Only free the old string if we actually have a new string. */
    cstr8_free(str);
    return newStr;
}

CSTR_API cstr8 cstr8_replace_range(cstr8 str, size_t replaceOffset, size_t replaceLength, const char* pOther, size_t otherLength)
{
    return cstr8_replace_range_ex(str, replaceOffset, replaceLength, pOther, otherLength, NULL, NULL);
}

CSTR_API cstr8 cstr8_replace_range_tagged(cstr8 str, const char* pTagBeg, const char* pTagEnd, const char* pOther, const char* pOtherTagBeg, const char* pOtherTagEnd, cstr_bool32 keepTagsOnSeparateLines)
{
    size_t strOffsetBeg;
    size_t strOffsetEnd;
    size_t otherSubstrLen;
    const char* pOtherSubstr;
    const char* pOtherNewLines = NULL;

    if (str == NULL || pOther == NULL) {
        return str;
    }

    if (pTagBeg == NULL || pTagBeg[0] == '\0') {
        strOffsetBeg = 0;
    } else {
        strOffsetBeg = cstr8_find(str, pTagBeg);
        if (strOffsetBeg == cstr_npos) {
            return str; /* Could not find begin tag. */
        } else {
            strOffsetBeg += utf8_strlen(pTagBeg);   /* Don't want to replace the tag itself. */
        }
    }

    if (pTagEnd == NULL || pTagEnd[0] == '\0') {
        strOffsetEnd = cstr8_len(str);
    } else {
        strOffsetEnd = cstr8_find(str + strOffsetBeg, pTagEnd);
        if (strOffsetEnd == cstr_npos) {
            return str; /* Could not find end tag. */
        } else {
            strOffsetEnd += strOffsetBeg;   /* When we searched for the string we started from the end of the beginning tag. Need to normalize the end offset. */
        }
    }

    pOtherSubstr = cstr8_substr_tagged(pOther, pOtherTagBeg, pOtherTagEnd, &otherSubstrLen);
    if (pOtherSubstr == NULL) {
        return str; /* Failed to retrieve the substring of the other string. */
    }

    if (keepTagsOnSeparateLines) {
        pOtherNewLines = "\n";
    }

    return cstr8_replace_range_ex(str, strOffsetBeg, strOffsetEnd - strOffsetBeg, pOtherSubstr, otherSubstrLen, pOtherNewLines, pOtherNewLines);
}

CSTR_API cstr8 cstr8_new_trim(const char* pOther)
{
    if (pOther == NULL) {
        return NULL;
    }

    return cstr8_newn_trim(pOther, (size_t)-1);
}

CSTR_API cstr8 cstr8_newn_trim(const char* pOther, size_t otherLen)
{
    size_t loff;
    size_t roff;

    if (pOther == NULL) {
        return NULL;
    }

    loff = cstr_utf8_ltrim_offset(pOther, otherLen);
    roff = cstr_utf8_rtrim_offset(pOther, otherLen);

    return cstr_newn(pOther + loff, roff - loff);
}

CSTR_API cstr8 cstr8_remove_at(cstr8 str, size_t index)
{
    if (str == NULL) {
        return NULL;
    }

    if (index >= cstr8_len(str)) {
        return str; /* Out of bounds. */
    }
    
    CSTR_MOVE_MEMORY(str + index, str + index + 1, cstr8_len(str) - index); /* This will also move the null terminator. */
    cstr8_set_len(str, cstr8_len(str) - 1);
    
    return str;
}
#endif /* CSTR_NO_UTF8 */


/**************************************************************************************************************************************************************

Unicode

**************************************************************************************************************************************************************/
static CSTR_INLINE cstr_bool32 cstr_is_invalid_utf8_octet(cstr_utf8 utf8)
{
    /* RFC 3629 - Section 1: The octet values C0, C1, F5 to FF never appear. */
    return (cstr_uint8)utf8 == 0xC0 || (cstr_uint8)utf8 == 0xC1 || (cstr_uint8)utf8 >= 0xF5;
}

static CSTR_INLINE void cstr_utf32_cp_to_utf16_pair(cstr_utf32 utf32cp, cstr_utf16* pUTF16)
{
    /* RFC 2781 - Section 2.1 */
    cstr_utf32 u;

    CSTR_ASSERT(utf32cp >= 0x10000);

    u = utf32cp - 0x10000;
    pUTF16[0] = (cstr_utf16)(0xD800 | ((u & 0xFFC00) >> 10));
    pUTF16[1] = (cstr_utf16)(0xDC00 | ((u & 0x003FF) >>  0));
}

static CSTR_INLINE cstr_utf32 cstr_utf16_pair_to_utf32_cp(const cstr_utf16* pUTF16)
{
    /* RFC 2781 - Section 2.1 */
    CSTR_ASSERT(pUTF16 != NULL);

    return (((cstr_utf32)(pUTF16[0] & 0x003FF) << 10) | ((cstr_utf32)(pUTF16[1] & 0x003FF) << 0)) + 0x10000;
}

static CSTR_INLINE cstr_bool32 cstr_is_cp_in_surrogate_pair_range(cstr_utf32 utf32)
{
    return utf32 >= 0xD800 && utf32 <= 0xDFFF;
}

static CSTR_INLINE cstr_bool32 cstr_is_valid_code_point(cstr_utf32 utf32)
{
    return utf32 <= CSTR_UNICODE_MAX_CODE_POINT && !cstr_is_cp_in_surrogate_pair_range(utf32);
}

static CSTR_INLINE size_t cstr_utf32_cp_to_utf8_len(cstr_utf32 utf32)
{
    /* This API assumes the the UTF-32 code point is valid. */
    CSTR_ASSERT(utf32 <= CSTR_UNICODE_MAX_CODE_POINT);
    CSTR_ASSERT(cstr_is_cp_in_surrogate_pair_range(utf32) == CSTR_FALSE);

    if (utf32 <= 0x7F) {
        return 1;
    }
    if (utf32 <= 0x7FF) {
        return 2;
    }
    if (utf32 <= 0xFFFF) {
        return 3;
    }
    if (utf32 <= 0x10FFFF) {
        return 4;
    }

    /* Invalid. This function assume's the UTF-32 code point is valid so do an assert. */
    CSTR_ASSERT(CSTR_FALSE);
    return 0; /* Invalid. */
}

static CSTR_INLINE size_t cstr_utf32_cp_to_utf8(cstr_utf32 utf32, cstr_utf8* pUTF8, size_t utf8Cap)
{
    /* This API assumes the the UTF-32 code point is valid. */
    CSTR_ASSERT(utf32 <= CSTR_UNICODE_MAX_CODE_POINT);
    CSTR_ASSERT(cstr_is_cp_in_surrogate_pair_range(utf32) == CSTR_FALSE);
    CSTR_ASSERT(utf8Cap > 0);

    if (utf32 <= 0x7F) {
        if (utf8Cap >= 1) {
            pUTF8[0] = (utf32 & 0x7F);
            return 1;
        }
    }
    if (utf32 <= 0x7FF) {
        if (utf8Cap >= 2) {
            pUTF8[0] = 0xC0 | (cstr_utf8)((utf32 & 0x07C0) >> 6);
            pUTF8[1] = 0x80 | (cstr_utf8) (utf32 & 0x003F);
            return 2;
        }
    }
    if (utf32 <= 0xFFFF) {
        if (utf8Cap >= 3) {
            pUTF8[0] = 0xE0 | (cstr_utf8)((utf32 & 0xF000) >> 12);
            pUTF8[1] = 0x80 | (cstr_utf8)((utf32 & 0x0FC0) >>  6);
            pUTF8[2] = 0x80 | (cstr_utf8) (utf32 & 0x003F);
            return 3;
        }
    }
    if (utf32 <= 0x10FFFF) {
        if (utf8Cap >= 4) {
            pUTF8[0] = 0xF0 | (cstr_utf8)((utf32 & 0x1C0000) >> 18);
            pUTF8[1] = 0x80 | (cstr_utf8)((utf32 & 0x03F000) >> 12);
            pUTF8[2] = 0x80 | (cstr_utf8)((utf32 & 0x000FC0) >>  6);
            pUTF8[3] = 0x80 | (cstr_utf8) (utf32 & 0x00003F);
            return 4;
        }
    }

    /* Getting here means there was not enough room in the output buffer. */
    return 0;
}

static CSTR_INLINE size_t cstr_utf32_cp_to_utf16_len(cstr_utf32 utf32)
{
    /* This API assumes the the UTF-32 code point is valid. */
    CSTR_ASSERT(utf32 <= CSTR_UNICODE_MAX_CODE_POINT);
    CSTR_ASSERT(cstr_is_cp_in_surrogate_pair_range(utf32) == CSTR_FALSE);

    if (utf32 <= 0xFFFF) {
        return 1;
    } else {
        return 2;
    }

    /* Unreachable. */
#if 0
    /* Invalid. This function assume's the UTF-32 code point is valid so do an assert. */
    CSTR_ASSERT(CSTR_FALSE);
    return 0; /* Invalid. */
#endif
}

static CSTR_INLINE size_t cstr_utf32_cp_to_utf16(cstr_utf32 utf32, cstr_utf16* pUTF16, size_t utf16Cap)
{
    /* This API assumes the the UTF-32 code point is valid. */
    CSTR_ASSERT(utf32 <= CSTR_UNICODE_MAX_CODE_POINT);
    CSTR_ASSERT(cstr_is_cp_in_surrogate_pair_range(utf32) == CSTR_FALSE);
    CSTR_ASSERT(utf16Cap > 0);

    if (utf32 <= 0xFFFF) {
        if (utf16Cap >= 1) {
            pUTF16[0] = (cstr_utf16)utf32;
            return 1;
        }
    } else {
        if (utf16Cap >= 2) {
            cstr_utf32_cp_to_utf16_pair(utf32, pUTF16);
            return 2;
        }
    }

    /* Getting here means there was not enough room in the output buffer. */
    return 0;
}





CSTR_API cstr_bool32 cstr_is_utf16_bom_le(const cstr_uint8 bom[2])
{
    /* RFC 2781 - Section 3.2 */
    return bom[0] == 0xFF && bom[1] == 0xFE;
}

CSTR_API cstr_bool32 cstr_is_utf16_bom_be(const cstr_uint8 bom[2])
{
    /* RFC 2781 - Section 3.2 */
    return bom[0] == 0xFE && bom[1] == 0xFF;
}

CSTR_API cstr_bool32 cstr_is_utf32_bom_le(const cstr_uint8 bom[4])
{
    return bom[0] == 0xFF && bom[1] == 0xFE && bom[2] == 0x00 && bom[3] == 0x00;
}

CSTR_API cstr_bool32 cstr_is_utf32_bom_be(const cstr_uint8 bom[4])
{
    return bom[0] == 0x00 && bom[1] == 0x00 && bom[2] == 0xFE && bom[3] == 0xFF;
}

CSTR_API cstr_bool32 cstr_has_utf8_bom(const cstr_uint8* pBytes, size_t len)
{
    if (pBytes == NULL) {
        return CSTR_FALSE;
    }

    if (len < 3) {
        return CSTR_FALSE;
    }

    return (pBytes[0] == 0xEF && pBytes[1] == 0xBB && pBytes[2] == 0xBF);
}

CSTR_API cstr_bool32 cstr_has_utf16_bom(const cstr_uint8* pBytes, size_t len)
{
    if (pBytes == NULL) {
        return CSTR_FALSE;
    }

    if (len < 2) {
        return CSTR_FALSE;
    }

    return cstr_is_utf16_bom_le(pBytes) || cstr_is_utf16_bom_be(pBytes);
}

CSTR_API cstr_bool32 cstr_has_utf32_bom(const cstr_uint8* pBytes, size_t len)
{
    if (pBytes == NULL) {
        return CSTR_FALSE;
    }

    if (len < 4) {
        return CSTR_FALSE;
    }

    return cstr_is_utf32_bom_le(pBytes) || cstr_is_utf32_bom_be(pBytes);
}


CSTR_API void cstr_swap_endian_utf16(cstr_utf16* pUTF16, size_t count)
{
    if (count == (size_t)-1) {
        size_t i;
        for (i = 0; i < count; ++i) {
            pUTF16[i] = cstr_swap_endian_uint16(pUTF16[i]);
        }
    } else {
        while (pUTF16[0] != 0) {
            pUTF16[0] = cstr_swap_endian_uint16(pUTF16[0]);
            pUTF16 += 1;
        }
    }
}

CSTR_API void cstr_swap_endian_utf32(cstr_utf32* pUTF32, size_t count)
{
    if (count == (size_t)-1) {
        size_t i;
        for (i = 0; i < count; ++i) {
            pUTF32[i] = cstr_swap_endian_uint32(pUTF32[i]);
        }
    } else {
        while (pUTF32[0] != 0) {
            pUTF32[0] = cstr_swap_endian_uint32(pUTF32[0]);
            pUTF32 += 1;
        }
    }
}


CSTR_API errno_t cstr_utf8_to_utf16_len(size_t* pUTF16Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags)
{
    errno_t result = CSTR_SUCCESS;
    size_t utf16Len = 0;

    if (pUTF16Len != NULL) {
        *pUTF16Len = 0;
    }

    if (pUTF8LenProcessed != NULL) {
        *pUTF8LenProcessed = 0;
    }

    if (pUTF8 == NULL) {
        return EINVAL;   /* Invalid input. */
    }

    if (utf8Len == 0 || pUTF8[0] == 0) {
        return CSTR_SUCCESS;   /* Empty input string. Length is always 0. */
    }

    /* Check for BOM. */
    if (cstr_has_utf8_bom((const cstr_uint8*)pUTF8, utf8Len)) {
        if ((flags & CSTR_FORBID_BOM) != 0) {
            return CSTR_EBOM;  /* Found a BOM, but it's forbidden. */
        }

        pUTF8 += 3; /* Skip past the BOM. */
        if (utf8Len != (size_t)-1) {
            utf8Len -= 3;
        }
    }

    if (utf8Len == (size_t)-1) {
        /* Null terminated string. */
        const cstr_utf8* pUTF8Original = pUTF8;
        for (;;) {
            if (pUTF8[0] == 0) {
                break;  /* Reached the end of the null terminated string. */
            }

            if ((cstr_uint8)pUTF8[0] < 128) {   /* ASCII character. */
                utf16Len += 1;
                pUTF8    += 1;
            } else {
                if (cstr_is_invalid_utf8_octet(pUTF8[0])) {
                    if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                        result = CSTR_ECODEPOINT;
                        break;
                    } else {
                        /* Replacement. */
                        utf16Len += CSTR_UNICODE_REPLACEMENT_CODE_POINT_LENGTH_UTF16;
                        pUTF8    += 1;
                    }
                } else {
                    if ((pUTF8[0] & 0xE0) == 0xC0) {
                        if (pUTF8[1] == 0) {
                            result = EINVAL; /* Input string is too short. */
                            break;
                        }

                        utf16Len += 1;  /* Can be at most 1 UTF-16. */
                        pUTF8    += 2;
                    } else if ((pUTF8[0] & 0xF0) == 0xE0) {
                        if (pUTF8[1] == 0 || pUTF8[2] == 0) {
                            result = EINVAL; /* Input string is too short. */
                            break;
                        }

                        utf16Len += 1;  /* Can be at most 1 UTF-16.*/
                        pUTF8    += 3;
                    } else if ((pUTF8[0] & 0xF8) == 0xF0) {
                        cstr_uint32 cp;
                        if (pUTF8[1] == 0 || pUTF8[2] == 0 || pUTF8[3] == 0) {
                            result = EINVAL; /* Input string is too short. */
                            break;
                        }

                        cp = ((cstr_utf32)(pUTF8[0] & 0x07) << 18) | ((cstr_utf32)(pUTF8[1] & 0x3F) << 12) | ((cstr_utf32)(pUTF8[2] & 0x3F) << 6) | (pUTF8[3] & 0x3F);
                        if (!cstr_is_valid_code_point(cp)) {
                            if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                                result = CSTR_ECODEPOINT;
                                break;
                            } else {
                                /* Replacement. */
                                utf16Len += CSTR_UNICODE_REPLACEMENT_CODE_POINT_LENGTH_UTF16;
                                pUTF8    += 4;
                            }
                        } else {
                            utf16Len += 2;  /* Must be at least 2 UTF-16s */
                            pUTF8    += 4;
                        }
                    } else {
                        if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                            result = CSTR_ECODEPOINT;
                            break;
                        } else {
                            /* Replacement. */
                            utf16Len += CSTR_UNICODE_REPLACEMENT_CODE_POINT_LENGTH_UTF16;
                            pUTF8    += 1;
                        }
                    }
                }
            }
        }

        if (pUTF8LenProcessed != NULL) {
            *pUTF8LenProcessed = (pUTF8 - pUTF8Original);
        }
    } else {
        /* Fixed length string. */
        size_t iUTF8;
        for (iUTF8 = 0; iUTF8 < utf8Len; /* Do nothing */) {
            if ((cstr_uint8)pUTF8[iUTF8+0] < 128) {   /* ASCII character. */
                utf16Len += 1;
                iUTF8    += 1;
            } else {
                if (cstr_is_invalid_utf8_octet(pUTF8[iUTF8+0])) {
                    if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                        result = CSTR_ECODEPOINT;
                        break;
                    } else {
                        /* Replacement. */
                        utf16Len += CSTR_UNICODE_REPLACEMENT_CODE_POINT_LENGTH_UTF16;
                        iUTF8    += 1;
                    }
                } else {
                    if ((pUTF8[iUTF8+0] & 0xE0) == 0xC0) {
                        if (iUTF8+1 > utf8Len) {
                            result = EINVAL;
                            break;
                        }

                        utf16Len += 1;  /* Can be at most 1 UTF-16.*/
                        iUTF8    += 2;
                    } else if ((pUTF8[iUTF8+0] & 0xF0) == 0xE0) {
                        if (iUTF8+2 > utf8Len) {
                            result = EINVAL;
                            break;
                        }

                        utf16Len += 1;  /* Can be at most 1 UTF-16.*/
                        iUTF8    += 3;
                    } else if ((pUTF8[iUTF8+0] & 0xF8) == 0xF0) {
                        cstr_uint32 cp;
                        if (iUTF8+3 > utf8Len) {
                            return EINVAL;
                        }

                        cp = ((cstr_utf32)(pUTF8[0] & 0x07) << 18) | ((cstr_utf32)(pUTF8[1] & 0x3F) << 12) | ((cstr_utf32)(pUTF8[2] & 0x3F) << 6) | (pUTF8[3] & 0x3F);
                        if (!cstr_is_valid_code_point(cp)) {
                            if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                                result = CSTR_ECODEPOINT;
                                break;
                            } else {
                                /* Replacement. */
                                utf16Len += CSTR_UNICODE_REPLACEMENT_CODE_POINT_LENGTH_UTF16;
                                iUTF8    += 4;
                            }
                        } else {
                            utf16Len += 2;  /* Must be at least 2 UTF-16s */
                            iUTF8    += 4;
                        }
                    } else {
                        if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                            result = CSTR_ECODEPOINT;
                            break;
                        } else {
                            /* Replacement. */
                            utf16Len += CSTR_UNICODE_REPLACEMENT_CODE_POINT_LENGTH_UTF16;
                            iUTF8    += 1;
                        }
                    }
                }
            }
        }

        if (pUTF8LenProcessed != NULL) {
            *pUTF8LenProcessed = iUTF8;
        }
    }

    if (pUTF16Len != NULL) {
        *pUTF16Len = utf16Len;
    }

    return result;
}

CSTR_API errno_t cstr_utf8_to_utf16ne(cstr_utf16* pUTF16, size_t utf16Cap, size_t* pUTF16Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags)
{
    errno_t result = CSTR_SUCCESS;
    size_t utf16CapOriginal = utf16Cap;

    if (pUTF16 == NULL) {
        return cstr_utf8_to_utf16_len(pUTF16Len, pUTF8, utf8Len, pUTF8LenProcessed, flags);
    }

    if (pUTF16Len != NULL) {
        *pUTF16Len = 0;
    }

    if (pUTF8LenProcessed != NULL) {
        *pUTF8LenProcessed = 0;
    }

    if (pUTF8 == NULL) {
        return EINVAL;
    }

    /* Check for BOM. */
    if (cstr_has_utf8_bom((const cstr_uint8*)pUTF8, utf8Len)) {
        if ((flags & CSTR_FORBID_BOM) != 0) {
            return CSTR_EBOM;   /* Found a BOM, but it's forbidden. */
        }

        pUTF8 += 3; /* Skip past the BOM. */
        if (utf8Len != (size_t)-1) {
            utf8Len -= 3;
        }
    }

    if (utf8Len == (size_t)-1) {
        /* Null terminated string. */
        const cstr_utf8* pUTF8Original = pUTF8;
        while (pUTF8[0] != 0) {
            if (utf16Cap == 1) {
                result = ENOMEM;
                break;
            }

            if ((cstr_uint8)pUTF8[0] < 128) {   /* ASCII character. */
                pUTF16[0] = pUTF8[0];
                pUTF16   += 1;
                utf16Cap -= 1;
                pUTF8    += 1;
            } else {
                if (cstr_is_invalid_utf8_octet(pUTF8[0])) {
                    if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                        result = CSTR_ECODEPOINT;
                        break;
                    } else {
                        /* Replacement. */
                        pUTF16[0] = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                        pUTF16   += CSTR_UNICODE_REPLACEMENT_CODE_POINT_LENGTH_UTF16;
                        utf16Cap -= CSTR_UNICODE_REPLACEMENT_CODE_POINT_LENGTH_UTF16;
                        pUTF8    += 1;
                    }
                } else {
                    if ((pUTF8[0] & 0xE0) == 0xC0) {
                        if (pUTF8[1] == 0) {
                            result = EINVAL; /* Input string is too short. */
                            break;
                        }

                        pUTF16[0] = ((cstr_utf16)(pUTF8[0] & 0x1F) <<  6) | (pUTF8[1] & 0x3F);
                        pUTF16   += 1;
                        utf16Cap -= 1;
                        pUTF8    += 2;
                    } else if ((pUTF8[0] & 0xF0) == 0xE0) {
                        if (pUTF8[1] == 0 || pUTF8[2] == 0) {
                            result = EINVAL; /* Input string is too short. */
                            break;
                        }

                        pUTF16[0] = ((cstr_utf16)(pUTF8[0] & 0x0F) << 12) | ((cstr_utf16)(pUTF8[1] & 0x3F) << 6) | (pUTF8[2] & 0x3F);
                        pUTF16   += 1;
                        utf16Cap -= 1;
                        pUTF8    += 3;
                    } else if ((pUTF8[0] & 0xF8) == 0xF0) {
                        if (utf16Cap < 2) {
                            break;  /* No enough room. */
                        } else {
                            cstr_uint32 cp;
                            if (pUTF8[1] == 0 || pUTF8[2] == 0 || pUTF8[3] == 0) {
                                result = EINVAL; /* Input string is too short. */
                                break;
                            }

                            cp = ((cstr_utf32)(pUTF8[0] & 0x07) << 18) | ((cstr_utf32)(pUTF8[1] & 0x3F) << 12) | ((cstr_utf32)(pUTF8[2] & 0x3F) << 6) | (pUTF8[3] & 0x3F);
                            if (!cstr_is_valid_code_point(cp)) {
                                if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                                    result = CSTR_ECODEPOINT;
                                    break;
                                } else {
                                    /* Replacement. */
                                    pUTF16[0] = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                                    pUTF16   += CSTR_UNICODE_REPLACEMENT_CODE_POINT_LENGTH_UTF16;
                                    utf16Cap -= CSTR_UNICODE_REPLACEMENT_CODE_POINT_LENGTH_UTF16;
                                    pUTF8    += 4;
                                }
                            } else {
                                cstr_utf32_cp_to_utf16_pair(cp, pUTF16);
                                pUTF16   += 2;
                                utf16Cap -= 2;
                                pUTF8    += 4;
                            }
                        }
                    } else {
                        if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                            result = CSTR_ECODEPOINT;
                            break;
                        } else {
                            /* Replacement. */
                            pUTF16[0] = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                            pUTF16   += CSTR_UNICODE_REPLACEMENT_CODE_POINT_LENGTH_UTF16;
                            utf16Cap -= CSTR_UNICODE_REPLACEMENT_CODE_POINT_LENGTH_UTF16;
                            pUTF8    += 1;
                        }
                    }
                }
            }
        }

        if (pUTF8LenProcessed != NULL) {
            *pUTF8LenProcessed = (pUTF8 - pUTF8Original);
        }
    } else {
        /* Fixed length string. */
        size_t iUTF8;
        for (iUTF8 = 0; iUTF8 < utf8Len; /* Do nothing */) {
            if (utf16Cap == 1) {
                result = ENOMEM;
                break;
            }

            if ((cstr_uint8)pUTF8[iUTF8+0] < 128) {   /* ASCII character. */
                pUTF16[0] = pUTF8[iUTF8+0];
                pUTF16   += 1;
                utf16Cap -= 1;
                iUTF8    += 1;
            } else {
                if (cstr_is_invalid_utf8_octet(pUTF8[iUTF8+0])) {
                    if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                        result = CSTR_ECODEPOINT;
                        break;
                    } else {
                        /* Replacement. */
                        pUTF16[0] = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                        pUTF16   += CSTR_UNICODE_REPLACEMENT_CODE_POINT_LENGTH_UTF16;
                        utf16Cap -= CSTR_UNICODE_REPLACEMENT_CODE_POINT_LENGTH_UTF16;
                        iUTF8    += 1;
                    }
                } else {
                    if ((pUTF8[iUTF8+0] & 0xE0) == 0xC0) {
                        if (iUTF8+1 > utf8Len) {
                            result = EINVAL;
                            break;
                        }

                        pUTF16[0] = ((cstr_utf16)(pUTF8[iUTF8+0] & 0x1F) <<  6) | (pUTF8[iUTF8+1] & 0x3F);
                        pUTF16   += 1;
                        utf16Cap -= 1;
                        iUTF8    += 2;
                    } else if ((pUTF8[iUTF8+0] & 0xF0) == 0xE0) {
                        if (iUTF8+2 > utf8Len) {
                            result = EINVAL;
                            break;
                        }

                        pUTF16[0] = ((cstr_utf16)(pUTF8[iUTF8+0] & 0x0F) << 12) | ((cstr_utf16)(pUTF8[iUTF8+1] & 0x3F) << 6) | (pUTF8[iUTF8+2] & 0x3F);
                        pUTF16   += 1;
                        utf16Cap -= 1;
                        iUTF8    += 3;
                    } else if ((pUTF8[iUTF8+0] & 0xF8) == 0xF0) {
                        if (utf16Cap < 2) {
                            break;  /* No enough room. */
                        } else {
                            cstr_uint32 cp;
                            if (iUTF8+3 > utf8Len) {
                                result = EINVAL;
                                break;
                            }

                            cp = ((cstr_utf32)(pUTF8[iUTF8+0] & 0x07) << 18) | ((cstr_utf32)(pUTF8[iUTF8+1] & 0x3F) << 12) | ((cstr_utf32)(pUTF8[iUTF8+2] & 0x3F) << 6) | (pUTF8[iUTF8+3] & 0x3F);
                            if (!cstr_is_valid_code_point(cp)) {
                                if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                                    result = CSTR_ECODEPOINT;
                                    break;
                                } else {
                                    /* Replacement. */
                                    pUTF16[0] = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                                    pUTF16   += CSTR_UNICODE_REPLACEMENT_CODE_POINT_LENGTH_UTF16;
                                    utf16Cap -= CSTR_UNICODE_REPLACEMENT_CODE_POINT_LENGTH_UTF16;
                                    iUTF8    += 4;
                                }
                            } else {
                                cstr_utf32_cp_to_utf16_pair(cp, pUTF16);
                                pUTF16   += 2;
                                utf16Cap -= 2;
                                iUTF8    += 4;
                            }
                        }
                    } else {
                        if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                            result = CSTR_ECODEPOINT;
                            break;
                        } else {
                            /* Replacement. */
                            pUTF16[0] = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                            pUTF16   += CSTR_UNICODE_REPLACEMENT_CODE_POINT_LENGTH_UTF16;
                            utf16Cap -= CSTR_UNICODE_REPLACEMENT_CODE_POINT_LENGTH_UTF16;
                            iUTF8    += 1;
                        }
                    }
                }
            }
        }

        if (pUTF8LenProcessed != NULL) {
            *pUTF8LenProcessed = iUTF8;
        }
    }

    /* Null terminate. */
    if (utf16Cap == 0) {
        result = ENOMEM;    /* Not enough room in the output buffer. */
    } else {
        pUTF16[0] = 0;
    }

    if (pUTF16Len != NULL) {
        *pUTF16Len = (utf16CapOriginal - utf16Cap);
    }

    return result;
}

CSTR_API errno_t cstr_utf8_to_utf16le(cstr_utf16* pUTF16, size_t utf16Cap, size_t* pUTF16Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags)
{
    errno_t result;
    size_t utf16Len;

    /* Always do a native endian conversion first, then byte swap if necessary. */
    result = cstr_utf8_to_utf16ne(pUTF16, utf16Cap, &utf16Len, pUTF8, utf8Len, pUTF8LenProcessed, flags);

    if (pUTF16Len != NULL) {
        *pUTF16Len = utf16Len;
    }

    if (result != CSTR_SUCCESS) {
        return result;
    }

    if (pUTF16 != NULL && !cstr_is_little_endian()) {
        cstr_swap_endian_utf16(pUTF16, utf16Len);
    }

    return CSTR_SUCCESS;
}

CSTR_API errno_t cstr_utf8_to_utf16be(cstr_utf16* pUTF16, size_t utf16Cap, size_t* pUTF16Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags)
{
    errno_t result;
    size_t utf16Len;

    /* Always do a native endian conversion first, then byte swap if necessary. */
    result = cstr_utf8_to_utf16ne(pUTF16, utf16Cap, &utf16Len, pUTF8, utf8Len, pUTF8LenProcessed, flags);

    if (pUTF16Len != NULL) {
        *pUTF16Len = utf16Len;
    }

    if (result != CSTR_SUCCESS) {
        return result;
    }

    if (pUTF16 != NULL && !cstr_is_big_endian()) {
        cstr_swap_endian_utf16(pUTF16, utf16Len);
    }

    return CSTR_SUCCESS;
}

CSTR_API errno_t cstr_utf8_to_utf32_len(size_t* pUTF32Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags)
{
    errno_t result = CSTR_SUCCESS;
    size_t utf32Len = 0;

    if (pUTF32Len != NULL) {
        *pUTF32Len = 0;
    }

    if (pUTF8LenProcessed != NULL) {
        *pUTF8LenProcessed = 0;
    }

    if (pUTF8 == NULL) {
        return EINVAL;   /* Invalid input. */
    }

    if (utf8Len == 0 || pUTF8[0] == 0) {
        return CSTR_SUCCESS;   /* Empty input string. Length is always 0. */
    }

    /* Check for BOM. */
    if (cstr_has_utf8_bom((const cstr_uint8*)pUTF8, utf8Len)) {
        if ((flags & CSTR_FORBID_BOM) != 0) {
            return CSTR_EBOM;  /* Found a BOM, but it's forbidden. */
        }

        pUTF8 += 3; /* Skip past the BOM. */
        if (utf8Len != (size_t)-1) {
            utf8Len -= 3;
        }
    }

    if (utf8Len == (size_t)-1) {
        /* Null terminated string. */
        const cstr_utf8* pUTF8Original = pUTF8;
        while (pUTF8[0] != 0) {
            utf32Len += 1;
            if ((cstr_uint8)pUTF8[0] < 128) {   /* ASCII character. */
                pUTF8 += 1;
            } else {
                if (cstr_is_invalid_utf8_octet(pUTF8[0])) {
                    if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                        result = CSTR_ECODEPOINT;
                        break;
                    } else {
                        /* Replacement. */
                        pUTF8 += 1;
                    }
                } else {
                    if ((pUTF8[0] & 0xE0) == 0xC0) {
                        if (pUTF8[1] == 0) {
                            result = EINVAL; /* Input string is too short. */
                            break;
                        }
                        pUTF8 += 2;
                    } else if ((pUTF8[0] & 0xF0) == 0xE0) {
                        if (pUTF8[1] == 0 || pUTF8[2] == 0) {
                            result = EINVAL; /* Input string is too short. */
                            break;
                        }
                        pUTF8 += 3;
                    } else if ((pUTF8[0] & 0xF8) == 0xF0) {
                        cstr_uint32 cp;
                        if (pUTF8[1] == 0 || pUTF8[2] == 0 || pUTF8[3] == 0) {
                            result = EINVAL; /* Input string is too short. */
                            break;
                        }
                        cp = ((cstr_utf32)(pUTF8[0] & 0x07) << 18) | ((cstr_utf32)(pUTF8[1] & 0x3F) << 12) | ((cstr_utf32)(pUTF8[2] & 0x3F) << 6) | (pUTF8[3] & 0x3F);
                        if (!cstr_is_valid_code_point(cp)) {
                            if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                                result = CSTR_ECODEPOINT;
                                break;
                            } else {
                                /* Replacement. */
                            }
                        }
                        pUTF8 += 4;
                    } else {
                        if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                            result = CSTR_ECODEPOINT;
                            break;
                        } else {
                            /* Replacement. */
                            pUTF8 += 1;
                        }
                    }
                }
            }
        }

        if (pUTF8LenProcessed != NULL) {
            *pUTF8LenProcessed = (pUTF8 - pUTF8Original);
        }
    } else {
        /* Fixed length string. */
        size_t iUTF8;
        for (iUTF8 = 0; iUTF8 < utf8Len; /* Do nothing */) {
            utf32Len += 1;
            if ((cstr_uint8)pUTF8[iUTF8+0] < 128) {   /* ASCII character. */
                iUTF8 += 1;
            } else {
                if (cstr_is_invalid_utf8_octet(pUTF8[iUTF8+0])) {
                    if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                        result = CSTR_ECODEPOINT;
                        break;
                    } else {
                        /* Replacement. */
                        iUTF8 += 1;
                    }
                } else {
                    if ((pUTF8[iUTF8+0] & 0xE0) == 0xC0) {
                        if (iUTF8+1 >= utf8Len) {
                            result = EINVAL;
                            break;
                        }
                        iUTF8 += 2;
                    } else if ((pUTF8[iUTF8+0] & 0xF0) == 0xE0) {
                        if (iUTF8+2 >= utf8Len) {
                            result = EINVAL;
                            break;
                        }
                        iUTF8 += 3;
                    } else if ((pUTF8[iUTF8+0] & 0xF8) == 0xF0) {
                        cstr_uint32 cp;
                        if (iUTF8+3 >= utf8Len) {
                            result = EINVAL;
                            break;
                        }
                        cp = ((cstr_utf32)(pUTF8[0] & 0x07) << 18) | ((cstr_utf32)(pUTF8[1] & 0x3F) << 12) | ((cstr_utf32)(pUTF8[2] & 0x3F) << 6) | (pUTF8[3] & 0x3F);
                        if (!cstr_is_valid_code_point(cp)) {
                            if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                                result = CSTR_ECODEPOINT;
                                break;
                            } else {
                                /* Replacement. */
                            }
                        }

                        iUTF8 += 4;
                    } else {
                        if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                            result = CSTR_ECODEPOINT;
                            break;
                        } else {
                            /* Replacement. */
                            iUTF8 += 1;
                        }
                    }
                }
            }
        }

        if (pUTF8LenProcessed != NULL) {
            *pUTF8LenProcessed = iUTF8;
        }
    }

    if (pUTF32Len != NULL) {
        *pUTF32Len = utf32Len;
    }

    return result;
}


CSTR_API errno_t cstr_utf8_to_utf32ne(cstr_utf32* pUTF32, size_t utf32Cap, size_t* pUTF32Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags)
{
    errno_t result = CSTR_SUCCESS;
    size_t utf32CapOriginal = utf32Cap;

    if (pUTF32 == NULL) {
        return cstr_utf8_to_utf32_len(pUTF32Len, pUTF8, utf8Len, pUTF8LenProcessed, flags);
    }

    if (pUTF32Len != NULL) {
        *pUTF32Len = 0;
    }

    if (pUTF8LenProcessed != NULL) {
        *pUTF8LenProcessed = 0;
    }

    if (pUTF8 == NULL) {
        return EINVAL;
    }

    /* Check for BOM. */
    if (cstr_has_utf8_bom((const cstr_uint8*)pUTF8, utf8Len)) {
        if ((flags & CSTR_FORBID_BOM) != 0) {
            return CSTR_EBOM;  /* Found a BOM, but it's forbidden. */
        }

        pUTF8 += 3; /* Skip past the BOM. */
        if (utf8Len != (size_t)-1) {
            utf8Len -= 3;
        }
    }

    if (utf8Len == (size_t)-1) {
        /* Null terminated string. */
        const cstr_utf8* pUTF8Original = pUTF8;
        while (pUTF8[0] != 0) {
            if (utf32Cap == 0) {
                result = ENOMEM;
                break;
            }

            if ((cstr_uint8)pUTF8[0] < 128) {   /* ASCII character. */
                pUTF32[0] = pUTF8[0];
                pUTF8 += 1;
            } else {
                if (cstr_is_invalid_utf8_octet(pUTF8[0])) {
                    if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                        result = CSTR_ECODEPOINT;
                        break;
                    } else {
                        /* Replacement. */
                        pUTF32[0] = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                        pUTF8 += 1;
                    }
                } else {
                    if ((pUTF8[0] & 0xE0) == 0xC0) {
                        if (pUTF8[1] == 0) {
                            result = EINVAL; /* Input string is too short. */
                            break;
                        }

                        pUTF32[0] = ((cstr_utf16)(pUTF8[0] & 0x1F) <<  6) | (pUTF8[1] & 0x3F);
                        pUTF8 += 2;
                    } else if ((pUTF8[0] & 0xF0) == 0xE0) {
                        if (pUTF8[1] == 0 || pUTF8[2] == 0) {
                            result = EINVAL; /* Input string is too short. */
                            break;
                        }

                        pUTF32[0] = ((cstr_utf16)(pUTF8[0] & 0x0F) << 12) | ((cstr_utf16)(pUTF8[1] & 0x3F) << 6) | (pUTF8[2] & 0x3F);
                        pUTF8 += 3;
                    } else if ((pUTF8[0] & 0xF8) == 0xF0) {
                        if (pUTF8[1] == 0 || pUTF8[2] == 0 || pUTF8[3] == 0) {
                            result = EINVAL; /* Input string is too short. */
                            break;
                        }

                        pUTF32[0] = ((cstr_utf32)(pUTF8[0] & 0x07) << 18) | ((cstr_utf32)(pUTF8[1] & 0x3F) << 12) | ((cstr_utf32)(pUTF8[2] & 0x3F) << 6) | (pUTF8[3] & 0x3F);
                        pUTF8 += 4;

                        if (!cstr_is_valid_code_point(pUTF32[0])) {
                            if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                                result = CSTR_ECODEPOINT;   /* No characters should be in the UTF-16 surrogate pair range. */
                                break;
                            } else {
                                /* Replacement. */
                                pUTF32[0] = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                            }
                        }
                    } else {
                        if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                            result = CSTR_ECODEPOINT;
                            break;
                        } else {
                            /* Replacement. */
                            pUTF32[0] = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                            pUTF8 += 1;
                        }
                    }
                }
            }

            pUTF32   += 1;
            utf32Cap -= 1;
        }

        if (pUTF8LenProcessed != NULL) {
            *pUTF8LenProcessed = (pUTF8 - pUTF8Original);
        }
    } else {
        /* Fixed length string. */
        size_t iUTF8;
        for (iUTF8 = 0; iUTF8 < utf8Len; /* Do nothing */) {
            if (utf32Cap == 0) {
                result = ENOMEM;
                break;
            }

            if ((cstr_uint8)pUTF8[iUTF8+0] < 128) {   /* ASCII character. */
                pUTF32[0] = pUTF8[iUTF8+0];
                iUTF8 += 1;
            } else {
                if (cstr_is_invalid_utf8_octet(pUTF8[iUTF8+0])) {
                    if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                        result = CSTR_ECODEPOINT;
                        break;
                    } else {
                        /* Replacement. */
                        pUTF32[0] = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                        iUTF8 += 1;
                    }
                } else {
                    if ((pUTF8[iUTF8+0] & 0xE0) == 0xC0) {
                        if (iUTF8+1 >= utf8Len) {
                            result = EINVAL;
                            break;
                        }

                        pUTF32[0] = ((cstr_utf16)(pUTF8[iUTF8+0] & 0x1F) <<  6) | (pUTF8[iUTF8+1] & 0x3F);
                        iUTF8 += 2;
                    } else if ((pUTF8[iUTF8+0] & 0xF0) == 0xE0) {
                        if (iUTF8+2 >= utf8Len) {
                            result = EINVAL;
                            break;
                        }

                        pUTF32[0] = ((cstr_utf16)(pUTF8[iUTF8+0] & 0x0F) << 12) | ((cstr_utf16)(pUTF8[iUTF8+1] & 0x3F) << 6) | (pUTF8[iUTF8+2] & 0x3F);
                        iUTF8 += 3;
                    } else if ((pUTF8[iUTF8+0] & 0xF8) == 0xF0) {
                        if (iUTF8+3 >= utf8Len) {
                            result = EINVAL;
                            break;
                        }

                        pUTF32[0] = ((cstr_utf32)(pUTF8[iUTF8+0] & 0x07) << 18) | ((cstr_utf32)(pUTF8[iUTF8+1] & 0x3F) << 12) | ((cstr_utf32)(pUTF8[iUTF8+2] & 0x3F) << 6) | (pUTF8[iUTF8+3] & 0x3F);
                        iUTF8 += 4;

                        if (!cstr_is_valid_code_point(pUTF32[0])) {
                            if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                                result = CSTR_ECODEPOINT;
                                break;
                            } else {
                                /* Replacement. */
                                pUTF32[0] = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                            }
                        }
                    } else {
                        if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                            result = CSTR_ECODEPOINT;
                            break;
                        } else {
                            /* Replacement. */
                            pUTF32[0] = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                            iUTF8 += 1;
                        }
                    }
                }
            }

            pUTF32   += 1;
            utf32Cap -= 1;
        }

        if (pUTF8LenProcessed != NULL) {
            *pUTF8LenProcessed = iUTF8;
        }
    }

    /* Null terminate. */
    if (utf32Cap == 0) {
        result = ENOMEM;    /* Not enough room in the output buffer. */
    } else {
        pUTF32[0] = 0;
    }

    if (pUTF32Len != NULL) {
        *pUTF32Len = (utf32CapOriginal - utf32Cap);
    }

    return result;
}

CSTR_API errno_t cstr_utf8_to_utf32le(cstr_utf32* pUTF32, size_t utf32Cap, size_t* pUTF32Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags)
{
    errno_t result;
    size_t utf32Len;

    /* Always do a native endian conversion first, then byte swap if necessary. */
    result = cstr_utf8_to_utf32ne(pUTF32, utf32Cap, &utf32Len, pUTF8, utf8Len, pUTF8LenProcessed, flags);

    if (pUTF32Len != NULL) {
        *pUTF32Len = utf32Len;
    }

    if (result != CSTR_SUCCESS) {
        return result;
    }

    if (pUTF32 != NULL && !cstr_is_little_endian()) {
        cstr_swap_endian_utf32(pUTF32, utf32Len);
    }

    return CSTR_SUCCESS;
}

CSTR_API errno_t cstr_utf8_to_utf32be(cstr_utf32* pUTF32, size_t utf32Cap, size_t* pUTF32Len, const cstr_utf8* pUTF8, size_t utf8Len, size_t* pUTF8LenProcessed, cstr_uint32 flags)
{
    errno_t result;
    size_t utf32Len;

    /* Always do a native endian conversion first, then byte swap if necessary. */
    result = cstr_utf8_to_utf32ne(pUTF32, utf32Cap, &utf32Len, pUTF8, utf8Len, pUTF8LenProcessed, flags);

    if (pUTF32Len != NULL) {
        *pUTF32Len = utf32Len;
    }

    if (result != CSTR_SUCCESS) {
        return result;
    }

    if (pUTF32 != NULL && !cstr_is_big_endian()) {
        cstr_swap_endian_utf32(pUTF32, utf32Len);
    }

    return CSTR_SUCCESS;
}



CSTR_API errno_t cstr_utf16_to_utf8_len_internal(size_t* pUTF8Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags, cstr_bool32 isLE)
{
    errno_t result = CSTR_SUCCESS;
    size_t utf8Len = 0;
    cstr_utf16 w1;
    cstr_utf16 w2;
    cstr_utf32 utf32;

    if (pUTF8Len != NULL) {
        *pUTF8Len = 0;
    }

    if (pUTF16LenProcessed != NULL) {
        *pUTF16LenProcessed = 0;
    }

    if (pUTF16 == NULL) {
        return EINVAL; /* Invalid input. */
    }

    if (utf16Len == 0 || pUTF16[0] == 0) {
        return CSTR_SUCCESS;  /* Empty input string. Length is always 0. */
    }

    /* Check for BOM. */
    if (cstr_has_utf16_bom((const cstr_uint8*)pUTF16, utf16Len)) {
        if ((flags & CSTR_FORBID_BOM) != 0) {
            return CSTR_EBOM;  /* Found a BOM, but it's forbidden. */
        }

        pUTF16 += 1; /* Skip past the BOM. */
        if (utf16Len != (size_t)-1) {
            utf16Len -= 1;
        }
    }

    if (utf16Len == (size_t)-1) {
        /* Null terminated string. */
        const cstr_utf16* pUTF16Original = pUTF16;
        while (pUTF16[0] != 0) {
            if (isLE) {
                w1 = cstr_le2host_16(pUTF16[0]);
            } else {
                w1 = cstr_be2host_16(pUTF16[0]);
            }
            
            if (w1 < 0xD800 || w1 > 0xDFFF) {
                /* 1 UTF-16 code unit. */
                utf32 = w1;
                pUTF16 += 1;
            } else {
                /* 2 UTF-16 code units, or an error. */
                if (w1 >= 0xD800 && w1 <= 0xDBFF) {
                    if (pUTF16[1] == 0) {
                        result = EINVAL; /* Ran out of input data. */
                        break;
                    } else {
                        if (isLE) {
                            w2 = cstr_le2host_16(pUTF16[1]);
                        } else {
                            w2 = cstr_be2host_16(pUTF16[1]);
                        }
                    
                        if (w2 >= 0xDC00 && w2 <= 0xDFFF) {
                            utf32 = cstr_utf16_pair_to_utf32_cp(pUTF16);
                        } else {
                            if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                                result = CSTR_ECODEPOINT;
                                break;
                            } else {
                                /* Replacement. */
                                utf32 = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                            }
                        }

                        pUTF16 += 2;
                    }
                } else {
                    if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                        result = CSTR_ECODEPOINT;
                        break;
                    } else {
                        /* Replacement. */
                        utf32 = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                    }

                    pUTF16 += 1;
                }
            }

            utf8Len += cstr_utf32_cp_to_utf8_len(utf32);
        }

        if (pUTF16LenProcessed != NULL) {
            *pUTF16LenProcessed = (pUTF16 - pUTF16Original);
        }
    } else {
        /* Fixed length string. */
        size_t iUTF16;
        for (iUTF16 = 0; iUTF16 < utf16Len; /* Do nothing */) {
            if (isLE) {
                w1 = cstr_le2host_16(pUTF16[iUTF16+0]);
            } else {
                w1 = cstr_be2host_16(pUTF16[iUTF16+0]);
            }

            if (w1 < 0xD800 || w1 > 0xDFFF) {
                /* 1 UTF-16 code unit. */
                utf32 = w1;
                iUTF16 += 1;
            } else {
                /* 2 UTF-16 code units, or an error. */
                if (w1 >= 0xD800 && w1 <= 0xDBFF) {
                    if (iUTF16+1 > utf16Len) {
                        result = EINVAL; /* Ran out of input data. */
                        break;
                    } else {
                        if (isLE) {
                            w2 = cstr_le2host_16(pUTF16[iUTF16+1]);
                        } else {
                            w2 = cstr_be2host_16(pUTF16[iUTF16+1]);
                        }
                    
                        if (w2 >= 0xDC00 && w2 <= 0xDFFF) {
                            utf32 = cstr_utf16_pair_to_utf32_cp(pUTF16 + iUTF16);
                        } else {
                            if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                                result = CSTR_ECODEPOINT;
                                break;
                            } else {
                                /* Replacement. */
                                utf32 = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                            }
                        }

                        iUTF16 += 2;
                    }
                } else {
                    if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                        result = CSTR_ECODEPOINT;
                        break;
                    } else {
                        utf32 = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                    }

                    iUTF16 += 1;
                }
            }

            utf8Len += cstr_utf32_cp_to_utf8_len(utf32);
        }

        if (pUTF16LenProcessed != NULL) {
            *pUTF16LenProcessed = iUTF16;
        }
    }

    if (pUTF8Len != NULL) {
        *pUTF8Len = utf8Len;
    }

    return result;
}

CSTR_API errno_t cstr_utf16ne_to_utf8_len(size_t* pUTF8Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags)
{
    if (cstr_is_little_endian()) {
        return cstr_utf16le_to_utf8_len(pUTF8Len, pUTF16, utf16Len, pUTF16LenProcessed, flags);
    } else {
        return cstr_utf16be_to_utf8_len(pUTF8Len, pUTF16, utf16Len, pUTF16LenProcessed, flags);
    }
}

CSTR_API errno_t cstr_utf16le_to_utf8_len(size_t* pUTF8Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags)
{
    return cstr_utf16_to_utf8_len_internal(pUTF8Len, pUTF16, utf16Len, pUTF16LenProcessed, flags, CSTR_TRUE);
}

CSTR_API errno_t cstr_utf16be_to_utf8_len(size_t* pUTF8Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags)
{
    return cstr_utf16_to_utf8_len_internal(pUTF8Len, pUTF16, utf16Len, pUTF16LenProcessed, flags, CSTR_FALSE);
}

CSTR_API errno_t cstr_utf16_to_utf8_len(size_t* pUTF8Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags)
{
    if (pUTF8Len != NULL) {
        *pUTF8Len = 0;
    }

    /* Check for BOM. */
    if (cstr_has_utf16_bom((const cstr_uint8*)pUTF16, utf16Len)) {
        errno_t result;
        size_t utf16LenProcessed;

        cstr_bool32 isLE;
        if ((flags & CSTR_FORBID_BOM) != 0) {
            return CSTR_EBOM;  /* Found a BOM, but it's forbidden. */
        }

        /* With this function, we need to use the endian defined by the BOM. */
        isLE = cstr_is_utf16_bom_le((const cstr_uint8*)pUTF16);
        
        pUTF16 += 1;    /* Skip past the BOM. */
        if (utf16Len != (size_t)-1) {
            utf16Len -= 1;
        }

        if (isLE) {
            result = cstr_utf16le_to_utf8_len(pUTF8Len, pUTF16+1, utf16Len-1, &utf16LenProcessed, flags | CSTR_FORBID_BOM); /* <-- We already found a BOM, so we don't want to allow another occurance. */
        } else {
            result = cstr_utf16be_to_utf8_len(pUTF8Len, pUTF16+1, utf16Len-1, &utf16LenProcessed, flags | CSTR_FORBID_BOM); /* <-- We already found a BOM, so we don't want to allow another occurance. */
        }

        if (pUTF16LenProcessed != NULL) {
            *pUTF16LenProcessed = utf16LenProcessed + 1;    /* +1 for BOM. */
        }

        return result;
    }

    /* Getting here means there was no BOM, so assume native endian. */
    return cstr_utf16ne_to_utf8_len(pUTF8Len, pUTF16, utf16Len, pUTF16LenProcessed, flags);
}


CSTR_API errno_t cstr_utf16_to_utf8_internal(cstr_utf8* pUTF8, size_t utf8Cap, size_t* pUTF8Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags, cstr_bool32 isLE)
{
    errno_t result = CSTR_SUCCESS;
    size_t utf8CapOriginal = utf8Cap;
    cstr_utf16 w1;
    cstr_utf16 w2;
    cstr_utf32 utf32;
    size_t utf8cpLen;   /* Code point length in UTF-8 code units. */

    if (pUTF8 == NULL) {
        return cstr_utf16_to_utf8_len_internal(pUTF8Len, pUTF16, utf16Len, pUTF16LenProcessed, flags, isLE);
    }

    if (pUTF8Len != NULL) {
        *pUTF8Len = 0;
    }

    if (pUTF16LenProcessed != NULL) {
        *pUTF16LenProcessed = 0;
    }

    if (pUTF8 == NULL) {
        return EINVAL;
    }

    /* Check for BOM. */
    if (cstr_has_utf16_bom((const cstr_uint8*)pUTF16, utf16Len)) {
        if ((flags & CSTR_FORBID_BOM) != 0) {
            return CSTR_EBOM;  /* Found a BOM, but it's forbidden. */
        }

        pUTF16 += 1;    /* Skip past the BOM. */
        if (utf16Len != (size_t)-1) {
            utf16Len -= 1;
        }
    }

    if (utf16Len == (size_t)-1) {
        /* Null terminated string. */
        const cstr_utf16* pUTF16Original = pUTF16;
        while (pUTF16[0] != 0) {
            if (utf8Cap == 0) {
                result = ENOMEM;
                break;
            }

            if (isLE) {
                w1 = cstr_le2host_16(pUTF16[0]);
            } else {
                w1 = cstr_be2host_16(pUTF16[0]);
            }
            
            if (w1 < 0xD800 || w1 > 0xDFFF) {
                /* 1 UTF-16 code unit. */
                utf32 = w1;
                pUTF16 += 1;
            } else {
                /* 2 UTF-16 code units, or an error. */
                if (w1 >= 0xD800 && w1 <= 0xDBFF) {
                    if (pUTF16[1] == 0) {
                        result = EINVAL; /* Ran out of input data. */
                        break;
                    } else {
                        if (isLE) {
                            w2 = cstr_le2host_16(pUTF16[1]);
                        } else {
                            w2 = cstr_be2host_16(pUTF16[1]);
                        }
                    
                        if (w2 >= 0xDC00 && w2 <= 0xDFFF) {
                            utf32 = cstr_utf16_pair_to_utf32_cp(pUTF16);
                        } else {
                            if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                                result = CSTR_ECODEPOINT;
                                break;
                            } else {
                                /* Replacement. */
                                utf32 = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                            }
                        }

                        pUTF16 += 2;
                    }
                } else {
                    if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                        result = CSTR_ECODEPOINT;
                        break;
                    } else {
                        /* Replacement. */
                        utf32 = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                    }

                    pUTF16 += 1;
                }
            }

            utf8cpLen = cstr_utf32_cp_to_utf8(utf32, pUTF8, utf8Cap);
            if (utf8cpLen == 0) {
                result = ENOMEM;    /* A return value of 0 at this point means there was not enough room in the output buffer. */
                break;
            }

            pUTF8   += utf8cpLen;
            utf8Cap -= utf8cpLen;
        }

        if (pUTF16LenProcessed != NULL) {
            *pUTF16LenProcessed = (pUTF16 - pUTF16Original);
        }
    } else {
        /* Fixed length string. */
        size_t iUTF16;
        for (iUTF16 = 0; iUTF16 < utf16Len; /* Do nothing */) {
            if (utf8Cap == 0) {
                result = ENOMEM;
                break;
            }

            if (isLE) {
                w1 = cstr_le2host_16(pUTF16[iUTF16+0]);
            } else {
                w1 = cstr_be2host_16(pUTF16[iUTF16+0]);
            }

            if (w1 < 0xD800 || w1 > 0xDFFF) {
                /* 1 UTF-16 code unit. */
                utf32 = w1;
                iUTF16 += 1;
            } else {
                /* 2 UTF-16 code units, or an error. */
                if (w1 >= 0xD800 && w1 <= 0xDBFF) {
                    if (iUTF16+1 > utf16Len) {
                        result = EINVAL; /* Ran out of input data. */
                        break;
                    } else {
                        if (isLE) {
                            w2 = cstr_le2host_16(pUTF16[iUTF16+1]);
                        } else {
                            w2 = cstr_be2host_16(pUTF16[iUTF16+1]);
                        }
                    
                        if (w2 >= 0xDC00 && w2 <= 0xDFFF) {
                            utf32 = cstr_utf16_pair_to_utf32_cp(pUTF16 + iUTF16);
                        } else {
                            if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                                result = CSTR_ECODEPOINT;
                                break;
                            } else {
                                /* Replacement. */
                                utf32 = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                            }
                        }

                        iUTF16 += 2;
                    }
                } else {
                    if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                        result = CSTR_ECODEPOINT;
                        break;
                    } else {
                        /* Replacement. */
                        utf32 = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                    }

                    iUTF16 += 1;
                }
            }

            utf8cpLen = cstr_utf32_cp_to_utf8(utf32, pUTF8, utf8Cap);
            if (utf8cpLen == 0) {
                result = ENOMEM;    /* A return value of 0 at this point means there was not enough room in the output buffer. */
                break;
            }

            pUTF8   += utf8cpLen;
            utf8Cap -= utf8cpLen;
        }

        if (pUTF16LenProcessed != NULL) {
            *pUTF16LenProcessed = iUTF16;
        }
    }
    
    /* Null terminate. */
    if (utf8Cap == 0) {
        result = ENOMEM;    /* Not enough room in the output buffer. */
    } else {
        pUTF8[0] = 0;
    }

    if (pUTF8Len != NULL) {
        *pUTF8Len = (utf8CapOriginal - utf8Cap);
    }

    return result;
}

CSTR_API errno_t cstr_utf16ne_to_utf8(cstr_utf8* pUTF8, size_t utf8Cap, size_t* pUTF8Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags)
{
    if (cstr_is_little_endian()) {
        return cstr_utf16le_to_utf8(pUTF8, utf8Cap, pUTF8Len, pUTF16, utf16Len, pUTF16LenProcessed, flags);
    } else {
        return cstr_utf16be_to_utf8(pUTF8, utf8Cap, pUTF8Len, pUTF16, utf16Len, pUTF16LenProcessed, flags);
    }
}

CSTR_API errno_t cstr_utf16le_to_utf8(cstr_utf8* pUTF8, size_t utf8Cap, size_t* pUTF8Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags)
{
    return cstr_utf16_to_utf8_internal(pUTF8, utf8Cap, pUTF8Len, pUTF16, utf16Len, pUTF16LenProcessed, flags, CSTR_TRUE);
}

CSTR_API errno_t cstr_utf16be_to_utf8(cstr_utf8* pUTF8, size_t utf8Cap, size_t* pUTF8Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags)
{
    return cstr_utf16_to_utf8_internal(pUTF8, utf8Cap, pUTF8Len, pUTF16, utf16Len, pUTF16LenProcessed, flags, CSTR_FALSE);
}

CSTR_API errno_t cstr_utf16_to_utf8(cstr_utf8* pUTF8, size_t utf8Cap, size_t* pUTF8Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags)
{
    if (pUTF8 == NULL) {
        return cstr_utf16_to_utf8_len(pUTF8Len, pUTF16, utf16Len, pUTF16LenProcessed, flags);
    }

    if (pUTF8Len != NULL) {
        *pUTF8Len = 0;
    }

    if (pUTF8 == NULL) {
        return EINVAL;
    }

    /* Check for BOM. */
    if (cstr_has_utf16_bom((const cstr_uint8*)pUTF16, utf16Len)) {
        errno_t result;
        size_t utf16LenProcessed;

        cstr_bool32 isLE;
        if ((flags & CSTR_FORBID_BOM) != 0) {
            return CSTR_EBOM;  /* Found a BOM, but it's forbidden. */
        }

        /* With this function, we need to use the endian defined by the BOM. */
        isLE = cstr_is_utf16_bom_le((const cstr_uint8*)pUTF16);
        
        pUTF16 += 1;    /* Skip past the BOM. */
        if (utf16Len != (size_t)-1) {
            utf16Len -= 1;
        }

        if (isLE) {
            result = cstr_utf16le_to_utf8(pUTF8, utf8Cap, pUTF8Len, pUTF16+1, utf16Len-1, &utf16LenProcessed, flags | CSTR_FORBID_BOM); /* <-- We already found a BOM, so we don't want to allow another occurance. */
        } else {
            result = cstr_utf16be_to_utf8(pUTF8, utf8Cap, pUTF8Len, pUTF16+1, utf16Len-1, &utf16LenProcessed, flags | CSTR_FORBID_BOM); /* <-- We already found a BOM, so we don't want to allow another occurance. */
        }

        if (pUTF16LenProcessed != NULL) {
            *pUTF16LenProcessed = utf16LenProcessed + 1;    /* +1 for BOM. */
        }

        return result;
    }

    /* Getting here means there was no BOM, so assume native endian. */
    return cstr_utf16ne_to_utf8(pUTF8, utf8Cap, pUTF8Len, pUTF16, utf16Len, pUTF16LenProcessed, flags);
}


CSTR_API errno_t cstr_utf16_to_utf32_len_internal(size_t* pUTF32Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags, cstr_bool32 isLE)
{
    errno_t result = CSTR_SUCCESS;
    size_t utf32Len = 0;
    cstr_utf16 w1;
    cstr_utf16 w2;

    if (pUTF32Len != NULL) {
        *pUTF32Len = 0;
    }

    if (pUTF16LenProcessed != NULL) {
        *pUTF16LenProcessed = 0;
    }

    if (pUTF16 == NULL) {
        return EINVAL; /* Invalid input. */
    }

    if (utf16Len == 0 || pUTF16[0] == 0) {
        return CSTR_SUCCESS;  /* Empty input string. Length is always 0. */
    }

    /* Check for BOM. */
    if (cstr_has_utf16_bom((const cstr_uint8*)pUTF16, utf16Len)) {
        if ((flags & CSTR_FORBID_BOM) != 0) {
            return CSTR_EBOM;  /* Found a BOM, but it's forbidden. */
        }

        pUTF16 += 1; /* Skip past the BOM. */
        if (utf16Len != (size_t)-1) {
            utf16Len -= 1;
        }
    }

    if (utf16Len == (size_t)-1) {
        /* Null terminated string. */
        const cstr_utf16* pUTF16Original = pUTF16;
        while (pUTF16[0] != 0) {
            if (isLE) {
                w1 = cstr_le2host_16(pUTF16[0]);
            } else {
                w1 = cstr_be2host_16(pUTF16[0]);
            }
            
            if (w1 < 0xD800 || w1 > 0xDFFF) {
                /* 1 UTF-16 code unit. */
                pUTF16 += 1;
            } else {
                /* 2 UTF-16 code units, or an error. */
                if (w1 >= 0xD800 && w1 <= 0xDBFF) {
                    if (pUTF16[1] == 0) {
                        result = EINVAL; /* Ran out of input data. */
                        break;
                    } else {
                        if (isLE) {
                            w2 = cstr_le2host_16(pUTF16[1]);
                        } else {
                            w2 = cstr_be2host_16(pUTF16[1]);
                        }
                    
                        /* Check for invalid code point. */
                        if (!(w2 >= 0xDC00 && w2 <= 0xDFFF)) {
                            if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                                result = CSTR_ECODEPOINT;
                                break;
                            }
                        }

                        pUTF16 += 2;
                    }
                } else {
                    if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                        result = CSTR_ECODEPOINT;
                        break;
                    }

                    pUTF16 += 1;
                }
            }

            utf32Len += 1;
        }

        if (pUTF16LenProcessed != NULL) {
            *pUTF16LenProcessed = (pUTF16 - pUTF16Original);
        }
    } else {
        /* Fixed length string. */
        size_t iUTF16;
        for (iUTF16 = 0; iUTF16 < utf16Len; /* Do nothing */) {
            if (isLE) {
                w1 = cstr_le2host_16(pUTF16[iUTF16+0]);
            } else {
                w1 = cstr_be2host_16(pUTF16[iUTF16+0]);
            }

            if (w1 < 0xD800 || w1 > 0xDFFF) {
                /* 1 UTF-16 code unit. */
                iUTF16 += 1;
            } else {
                /* 2 UTF-16 code units, or an error. */
                if (w1 >= 0xD800 && w1 <= 0xDBFF) {
                    if (iUTF16+1 > utf16Len) {
                        result = EINVAL; /* Ran out of input data. */
                        break;
                    } else {
                        if (isLE) {
                            w2 = cstr_le2host_16(pUTF16[iUTF16+1]);
                        } else {
                            w2 = cstr_be2host_16(pUTF16[iUTF16+1]);
                        }
                    
                        /* Check for invalid code point. */
                        if (!(w2 >= 0xDC00 && w2 <= 0xDFFF)) {
                            if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                                result = CSTR_ECODEPOINT;
                                break;
                            }
                        }

                        iUTF16 += 2;
                    }
                } else {
                    if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                        result = CSTR_ECODEPOINT;
                        break;
                    }

                    iUTF16 += 1;
                }
            }

            utf32Len += 1;
        }

        if (pUTF16LenProcessed != NULL) {
            *pUTF16LenProcessed = iUTF16;
        }
    }

    if (pUTF32Len != NULL) {
        *pUTF32Len = utf32Len;
    }

    return result;
}

CSTR_API errno_t cstr_utf16ne_to_utf32_len(size_t* pUTF32Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags)
{
    if (cstr_is_little_endian()) {
        return cstr_utf16le_to_utf32_len(pUTF32Len, pUTF16, utf16Len, pUTF16LenProcessed, flags);
    } else {
        return cstr_utf16be_to_utf32_len(pUTF32Len, pUTF16, utf16Len, pUTF16LenProcessed, flags);
    }
}

CSTR_API errno_t cstr_utf16le_to_utf32_len(size_t* pUTF32Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags)
{
    return cstr_utf16_to_utf32_len_internal(pUTF32Len, pUTF16, utf16Len, pUTF16LenProcessed, flags, CSTR_TRUE);
}

CSTR_API errno_t cstr_utf16be_to_utf32_len(size_t* pUTF32Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags)
{
    return cstr_utf16_to_utf32_len_internal(pUTF32Len, pUTF16, utf16Len, pUTF16LenProcessed, flags, CSTR_FALSE);
}

CSTR_API errno_t cstr_utf16_to_utf32_len(size_t* pUTF32Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags)
{
    if (pUTF32Len != NULL) {
        *pUTF32Len = 0;
    }

    /* Check for BOM. */
    if (cstr_has_utf16_bom((const cstr_uint8*)pUTF16, utf16Len)) {
        errno_t result;
        size_t utf16LenProcessed;

        cstr_bool32 isLE;
        if ((flags & CSTR_FORBID_BOM) != 0) {
            return CSTR_EBOM;  /* Found a BOM, but it's forbidden. */
        }

        /* With this function, we need to use the endian defined by the BOM. */
        isLE = cstr_is_utf16_bom_le((const cstr_uint8*)pUTF16);
        
        pUTF16 += 1;    /* Skip past the BOM. */
        if (utf16Len != (size_t)-1) {
            utf16Len -= 1;
        }

        if (isLE) {
            result = cstr_utf16le_to_utf32_len(pUTF32Len, pUTF16+1, utf16Len-1, &utf16LenProcessed, flags | CSTR_FORBID_BOM); /* <-- We already found a BOM, so we don't want to allow another occurance. */
        } else {
            result = cstr_utf16be_to_utf32_len(pUTF32Len, pUTF16+1, utf16Len-1, &utf16LenProcessed, flags | CSTR_FORBID_BOM); /* <-- We already found a BOM, so we don't want to allow another occurance. */
        }

        if (pUTF16LenProcessed != NULL) {
            *pUTF16LenProcessed = utf16LenProcessed + 1;
        }

        return result;
    }

    /* Getting here means there was no BOM, so assume native endian. */
    return cstr_utf16ne_to_utf32_len(pUTF32Len, pUTF16, utf16Len, pUTF16LenProcessed, flags);
}


CSTR_API errno_t cstr_utf16_to_utf32_internal(cstr_utf32* pUTF32, size_t utf32Cap, size_t* pUTF32Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags, cstr_bool32 isLE)
{
    errno_t result = CSTR_SUCCESS;
    size_t utf32CapOriginal = utf32Cap;
    cstr_utf16 w1;
    cstr_utf16 w2;
    cstr_utf32 utf32;

    if (pUTF32 == NULL) {
        return cstr_utf16_to_utf32_len_internal(pUTF32Len, pUTF16, utf16Len, pUTF16LenProcessed, flags, isLE);
    }

    if (pUTF32Len != NULL) {
        *pUTF32Len = 0;
    }

    if (pUTF16LenProcessed != NULL) {
        *pUTF16LenProcessed = 0;
    }

    if (pUTF32 == NULL) {
        return EINVAL;
    }

    /* Check for BOM. */
    if (cstr_has_utf16_bom((const cstr_uint8*)pUTF16, utf16Len)) {
        if ((flags & CSTR_FORBID_BOM) != 0) {
            return CSTR_EBOM;  /* Found a BOM, but it's forbidden. */
        }

        pUTF16 += 1;    /* Skip past the BOM. */
        if (utf16Len != (size_t)-1) {
            utf16Len -= 1;
        }
    }

    if (utf16Len == (size_t)-1) {
        /* Null terminated string. */
        const cstr_utf16* pUTF16Original = pUTF16;
        while (pUTF16[0] != 0) {
            if (utf32Cap == 0) {
                result = ENOMEM;
                break;
            }

            if (isLE) {
                w1 = cstr_le2host_16(pUTF16[0]);
            } else {
                w1 = cstr_be2host_16(pUTF16[0]);
            }
            
            if (w1 < 0xD800 || w1 > 0xDFFF) {
                /* 1 UTF-16 code unit. */
                utf32 = w1;
                pUTF16 += 1;
            } else {
                /* 2 UTF-16 code units, or an error. */
                if (w1 >= 0xD800 && w1 <= 0xDBFF) {
                    if (pUTF16[1] == 0) {
                        result = EINVAL; /* Ran out of input data. */
                        break;
                    } else {
                        if (isLE) {
                            w2 = cstr_le2host_16(pUTF16[1]);
                        } else {
                            w2 = cstr_be2host_16(pUTF16[1]);
                        }
                    
                        if (w2 >= 0xDC00 && w2 <= 0xDFFF) {
                            utf32 = cstr_utf16_pair_to_utf32_cp(pUTF16);
                        } else {
                            if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                                result = CSTR_ECODEPOINT;
                                break;
                            } else {
                                /* Replacement. */
                                utf32 = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                            }
                        }

                        pUTF16 += 2;
                    }
                } else {
                    if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                        result = CSTR_ECODEPOINT;
                        break;
                    } else {
                        /* Replacement. */
                        utf32 = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                    }

                    pUTF16 += 1;
                }
            }

            if (isLE) {
                pUTF32[0] = cstr_host2le_32(utf32);
            } else {
                pUTF32[0] = cstr_host2be_32(utf32);
            }
            
            pUTF32   += 1;
            utf32Cap -= 1;
        }

        if (pUTF16LenProcessed != NULL) {
            *pUTF16LenProcessed = (pUTF16 - pUTF16Original);
        }
    } else {
        /* Fixed length string. */
        size_t iUTF16;
        for (iUTF16 = 0; iUTF16 < utf16Len; /* Do nothing */) {
            if (utf32Cap == 0) {
                result = ENOMEM;
                break;
            }

            if (isLE) {
                w1 = cstr_le2host_16(pUTF16[iUTF16+0]);
            } else {
                w1 = cstr_be2host_16(pUTF16[iUTF16+0]);
            }

            if (w1 < 0xD800 || w1 > 0xDFFF) {
                /* 1 UTF-16 code unit. */
                utf32 = w1;
                iUTF16 += 1;
            } else {
                /* 2 UTF-16 code units, or an error. */
                if (w1 >= 0xD800 && w1 <= 0xDBFF) {
                    if (iUTF16+1 > utf16Len) {
                        result = EINVAL; /* Ran out of input data. */
                        break;
                    } else {
                        if (isLE) {
                            w2 = cstr_le2host_16(pUTF16[iUTF16+1]);
                        } else {
                            w2 = cstr_be2host_16(pUTF16[iUTF16+1]);
                        }
                    
                        if (w2 >= 0xDC00 && w2 <= 0xDFFF) {
                            utf32 = cstr_utf16_pair_to_utf32_cp(pUTF16 + iUTF16);
                        } else {
                            if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                                result = CSTR_ECODEPOINT;
                                break;
                            } else {
                                /* Replacement. */
                                utf32 = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                            }
                        }

                        iUTF16 += 2;
                    }
                } else {
                    if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                        result = CSTR_ECODEPOINT;
                        break;
                    } else {
                        /* Replacement. */
                        utf32 = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                    }

                    iUTF16 += 1;
                }
            }

            if (isLE) {
                pUTF32[0] = cstr_host2le_32(utf32);
            } else {
                pUTF32[0] = cstr_host2be_32(utf32);
            }

            pUTF32   += 1;
            utf32Cap -= 1;
        }

        if (pUTF16LenProcessed != NULL) {
            *pUTF16LenProcessed = iUTF16;
        }
    }
    
    /* Null terminate. */
    if (utf32Cap == 0) {
        result = ENOMEM;    /* Not enough room in the output buffer. */
    } else {
        pUTF32[0] = 0;
    }

    if (pUTF32Len != NULL) {
        *pUTF32Len = (utf32CapOriginal - utf32Cap);
    }

    return result;
}

CSTR_API errno_t cstr_utf16ne_to_utf32ne(cstr_utf32* pUTF32, size_t utf32Cap, size_t* pUTF32Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags)
{
    if (cstr_is_little_endian()) {
        return cstr_utf16le_to_utf32le(pUTF32, utf32Cap, pUTF32Len, pUTF16, utf16Len, pUTF16LenProcessed, flags);
    } else {
        return cstr_utf16be_to_utf32be(pUTF32, utf32Cap, pUTF32Len, pUTF16, utf16Len, pUTF16LenProcessed, flags);
    }
}

CSTR_API errno_t cstr_utf16le_to_utf32le(cstr_utf32* pUTF32, size_t utf32Cap, size_t* pUTF32Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags)
{
    return cstr_utf16_to_utf32_internal(pUTF32, utf32Cap, pUTF32Len, pUTF16, utf16Len, pUTF16LenProcessed, flags, CSTR_TRUE);
}

CSTR_API errno_t cstr_utf16be_to_utf32be(cstr_utf32* pUTF32, size_t utf32Cap, size_t* pUTF32Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags)
{
    return cstr_utf16_to_utf32_internal(pUTF32, utf32Cap, pUTF32Len, pUTF16, utf16Len, pUTF16LenProcessed, flags, CSTR_FALSE);
}

CSTR_API errno_t cstr_utf16_to_utf32(cstr_utf32* pUTF32, size_t utf32Cap, size_t* pUTF32Len, const cstr_utf16* pUTF16, size_t utf16Len, size_t* pUTF16LenProcessed, cstr_uint32 flags)
{
    if (pUTF32 == NULL) {
        return cstr_utf16_to_utf32_len(pUTF32Len, pUTF16, utf16Len, pUTF16LenProcessed, flags);
    }

    if (pUTF32Len != NULL) {
        *pUTF32Len = 0;
    }

    if (pUTF32 == NULL) {
        return EINVAL;
    }

    /* Check for BOM. */
    if (cstr_has_utf16_bom((const cstr_uint8*)pUTF16, utf16Len)) {
        errno_t result;
        size_t utf16LenProcessed;

        cstr_bool32 isLE;
        if ((flags & CSTR_FORBID_BOM) != 0) {
            return CSTR_EBOM;  /* Found a BOM, but it's forbidden. */
        }

        /* With this function, we need to use the endian defined by the BOM. */
        isLE = cstr_is_utf16_bom_le((const cstr_uint8*)pUTF16);
        
        pUTF16 += 1;    /* Skip past the BOM. */
        if (utf16Len != (size_t)-1) {
            utf16Len -= 1;
        }

        if (isLE) {
            result = cstr_utf16le_to_utf32le(pUTF32, utf32Cap, pUTF32Len, pUTF16+1, utf16Len-1, &utf16LenProcessed, flags | CSTR_FORBID_BOM); /* <-- We already found a BOM, so we don't want to allow another occurance. */
        } else {
            result = cstr_utf16be_to_utf32be(pUTF32, utf32Cap, pUTF32Len, pUTF16+1, utf16Len-1, &utf16LenProcessed, flags | CSTR_FORBID_BOM); /* <-- We already found a BOM, so we don't want to allow another occurance. */
        }

        if (pUTF16LenProcessed != NULL) {
            *pUTF16LenProcessed = utf16LenProcessed + 1;    /* +1 for BOM. */
        }

        return result;
    }

    /* Getting here means there was no BOM, so assume native endian. */
    return cstr_utf16ne_to_utf32ne(pUTF32, utf32Cap, pUTF32Len, pUTF16, utf16Len, pUTF16LenProcessed, flags);
}


CSTR_API errno_t cstr_utf32_to_utf8_len_internal(size_t* pUTF8Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags, cstr_bool32 isLE)
{
    errno_t result = CSTR_SUCCESS;
    size_t utf8Len = 0;
    cstr_utf32 utf32;

    if (pUTF8Len != NULL) {
        *pUTF8Len = 0;
    }

    if (pUTF32LenProcessed != NULL) {
        *pUTF32LenProcessed = 0;
    }

    if (pUTF32 == NULL) {
        return EINVAL; /* Invalid input. */
    }

    if (utf32Len == 0 || pUTF32[0] == 0) {
        return CSTR_SUCCESS;  /* Empty input string. Length is always 0. */
    }

    /* Check for BOM. */
    if (cstr_has_utf32_bom((const cstr_uint8*)pUTF32, utf32Len)) {
        if ((flags & CSTR_FORBID_BOM) != 0) {
            return CSTR_EBOM;  /* Found a BOM, but it's forbidden. */
        }

        pUTF32 += 1;    /* Skip past the BOM. */
        if (utf32Len != (size_t)-1) {
            utf32Len -= 1;
        }
    }

    if (utf32Len == (size_t)-1) {
        /* Null terminated string. */
        const cstr_utf32* pUTF32Original = pUTF32;
        while (pUTF32[0] != 0) {
            if (isLE) {
                utf32 = cstr_le2host_32(pUTF32[0]);
            } else {
                utf32 = cstr_be2host_32(pUTF32[0]);
            }

            if (!cstr_is_valid_code_point(utf32)) {
                if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                    result = CSTR_ECODEPOINT;
                    break;
                } else {
                    utf32 = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                }
            }

            utf8Len += cstr_utf32_cp_to_utf8_len(utf32);
            pUTF32  += 1;
        }

        if (pUTF32LenProcessed != NULL) {
            *pUTF32LenProcessed = (pUTF32 - pUTF32Original);
        }
    } else {
        /* Fixed length string. */
        size_t iUTF32;
        for (iUTF32 = 0; iUTF32 < utf32Len; iUTF32 += 1) {
            if (isLE) {
                utf32 = cstr_le2host_32(pUTF32[iUTF32]);
            } else {
                utf32 = cstr_be2host_32(pUTF32[iUTF32]);
            }

            if (!cstr_is_valid_code_point(utf32)) {
                if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                    result = CSTR_ECODEPOINT;
                    break;
                } else {
                    utf32 = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                }
            }

            utf8Len += cstr_utf32_cp_to_utf8_len(utf32);
        }

        if (pUTF32LenProcessed != NULL) {
            *pUTF32LenProcessed = iUTF32;
        }
    }

    if (pUTF8Len != NULL) {
        *pUTF8Len = utf8Len;
    }

    return result;
}

CSTR_API errno_t cstr_utf32ne_to_utf8_len(size_t* pUTF8Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags)
{
    if (cstr_is_little_endian()) {
        return cstr_utf32le_to_utf8_len(pUTF8Len, pUTF32, utf32Len, pUTF32LenProcessed, flags);
    } else {
        return cstr_utf32be_to_utf8_len(pUTF8Len, pUTF32, utf32Len, pUTF32LenProcessed, flags);
    }
}

CSTR_API errno_t cstr_utf32le_to_utf8_len(size_t* pUTF8Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags)
{
    return cstr_utf32_to_utf8_len_internal(pUTF8Len, pUTF32, utf32Len, pUTF32LenProcessed, flags, CSTR_TRUE);
}

CSTR_API errno_t cstr_utf32be_to_utf8_len(size_t* pUTF8Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags)
{
    return cstr_utf32_to_utf8_len_internal(pUTF8Len, pUTF32, utf32Len, pUTF32LenProcessed, flags, CSTR_FALSE);
}

CSTR_API errno_t cstr_utf32_to_utf8_len(size_t* pUTF8Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags)
{
    if (pUTF8Len != NULL) {
        *pUTF8Len = 0;
    }

    /* Check for BOM. */
    if (cstr_has_utf32_bom((const cstr_uint8*)pUTF32, utf32Len)) {
        errno_t result;
        size_t utf32LenProcessed;

        cstr_bool32 isLE;
        if ((flags & CSTR_FORBID_BOM) != 0) {
            return CSTR_EBOM;  /* Found a BOM, but it's forbidden. */
        }

        /* With this function, we need to use the endian defined by the BOM. */
        isLE = cstr_is_utf32_bom_le((const cstr_uint8*)pUTF32);
        
        pUTF32 += 1;    /* Skip past the BOM. */
        if (utf32Len != (size_t)-1) {
            utf32Len -= 1;
        }

        if (isLE) {
            result = cstr_utf32le_to_utf8_len(pUTF8Len, pUTF32+1, utf32Len-1, &utf32LenProcessed, flags | CSTR_FORBID_BOM); /* <-- We already found a BOM, so we don't want to allow another occurance. */
        } else {
            result = cstr_utf32be_to_utf8_len(pUTF8Len, pUTF32+1, utf32Len-1, &utf32LenProcessed, flags | CSTR_FORBID_BOM); /* <-- We already found a BOM, so we don't want to allow another occurance. */
        }

        if (pUTF32LenProcessed) {
            *pUTF32LenProcessed = utf32LenProcessed + 1;    /* +1 for the BOM. */
        }

        return result;
    }

    /* Getting here means there was no BOM, so assume native endian. */
    return cstr_utf32ne_to_utf8_len(pUTF8Len, pUTF32, utf32Len, pUTF32LenProcessed, flags);
}


CSTR_API errno_t cstr_utf32_to_utf8_internal(cstr_utf8* pUTF8, size_t utf8Cap, size_t* pUTF8Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags, cstr_bool32 isLE)
{
    errno_t result = CSTR_SUCCESS;
    size_t utf8CapOriginal = utf8Cap;
    size_t utf8cpLen;   /* Code point length in UTF-8 code units. */
    cstr_utf32 utf32;

    if (pUTF8 == NULL) {
        return cstr_utf32_to_utf8_internal(pUTF8, utf8Cap, pUTF8Len, pUTF32, utf32Len, pUTF32LenProcessed, flags, isLE);
    }

    if (pUTF8Len != NULL) {
        *pUTF8Len = 0;
    }

    if (pUTF32LenProcessed != NULL) {
        *pUTF32LenProcessed = 0;
    }

    if (pUTF8 == NULL) {
        return EINVAL;
    }

    /* Check for BOM. */
    if (cstr_has_utf32_bom((const cstr_uint8*)pUTF32, utf32Len)) {
        if ((flags & CSTR_FORBID_BOM) != 0) {
            return CSTR_EBOM;  /* Found a BOM, but it's forbidden. */
        }

        pUTF32 += 1;    /* Skip past the BOM. */
        if (utf32Len != (size_t)-1) {
            utf32Len -= 1;
        }
    }

    if (utf32Len == (size_t)-1) {
        /* Null terminated string. */
        const cstr_utf32* pUTF32Original = pUTF32;
        while (pUTF32[0] != 0) {
            if (utf8Cap == 0) {
                result = ENOMEM;
                break;
            }

            if (isLE) {
                utf32 = cstr_le2host_32(pUTF32[0]);
            } else {
                utf32 = cstr_be2host_32(pUTF32[0]);
            }

            if (!cstr_is_valid_code_point(utf32)) {
                if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                    result = CSTR_ECODEPOINT;
                    break;
                } else {
                    utf32 = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                }
            }

            utf8cpLen = cstr_utf32_cp_to_utf8(utf32, pUTF8, utf8Cap);
            if (utf8cpLen == 0) {
                result = ENOMEM;    /* A return value of 0 at this point means there was not enough room in the output buffer. */
                break;
            }

            pUTF8   += utf8cpLen;
            utf8Cap -= utf8cpLen;
            pUTF32  += 1;
        }

        if (pUTF32LenProcessed != NULL) {
            *pUTF32LenProcessed = (pUTF32 - pUTF32Original);
        }
    } else {
        /* Fixed length string. */
        size_t iUTF32;
        for (iUTF32 = 0; iUTF32 < utf32Len; iUTF32 += 1) {
            if (utf8Cap == 0) {
                result = ENOMEM;
                break;
            }

            if (isLE) {
                utf32 = cstr_le2host_32(pUTF32[iUTF32]);
            } else {
                utf32 = cstr_be2host_32(pUTF32[iUTF32]);
            }

            if (!cstr_is_valid_code_point(utf32)) {
                if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                    result = CSTR_ECODEPOINT;
                    break;
                } else {
                    utf32 = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                }
            }

            utf8cpLen = cstr_utf32_cp_to_utf8(utf32, pUTF8, utf8Cap);
            if (utf8cpLen == 0) {
                result = ENOMEM;    /* A return value of 0 at this point means there was not enough room in the output buffer. */
                break;
            }

            pUTF8   += utf8cpLen;
            utf8Cap -= utf8cpLen;
        }

        if (pUTF32LenProcessed != NULL) {
            *pUTF32LenProcessed = iUTF32;
        }
    }

    /* Null terminate. */
    if (utf8Cap == 0) {
        result = ENOMEM;    /* Not enough room in the output buffer. */
    } else {
        pUTF8[0] = 0;
    }

    if (pUTF8Len != NULL) {
        *pUTF8Len = (utf8CapOriginal - utf8Cap);
    }

    return result;
}

CSTR_API errno_t cstr_utf32ne_to_utf8(cstr_utf8* pUTF8, size_t utf8Cap, size_t* pUTF8Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags)
{
    if (cstr_is_little_endian()) {
        return cstr_utf32le_to_utf8(pUTF8, utf8Cap, pUTF8Len, pUTF32, utf32Len, pUTF32LenProcessed, flags);
    } else {
        return cstr_utf32be_to_utf8(pUTF8, utf8Cap, pUTF8Len, pUTF32, utf32Len, pUTF32LenProcessed, flags);
    }
}

CSTR_API errno_t cstr_utf32le_to_utf8(cstr_utf8* pUTF8, size_t utf8Cap, size_t* pUTF8Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags)
{
    return cstr_utf32_to_utf8_internal(pUTF8, utf8Cap, pUTF8Len, pUTF32, utf32Len, pUTF32LenProcessed, flags, CSTR_TRUE);
}

CSTR_API errno_t cstr_utf32be_to_utf8(cstr_utf8* pUTF8, size_t utf8Cap, size_t* pUTF8Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags)
{
    return cstr_utf32_to_utf8_internal(pUTF8, utf8Cap, pUTF8Len, pUTF32, utf32Len, pUTF32LenProcessed, flags, CSTR_FALSE);
}

CSTR_API errno_t cstr_utf32_to_utf8(cstr_utf8* pUTF8, size_t utf8Cap, size_t* pUTF8Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags)
{
    if (pUTF8Len != NULL) {
        *pUTF8Len = 0;
    }

    /* Check for BOM. */
    if (cstr_has_utf32_bom((const cstr_uint8*)pUTF32, utf32Len)) {
        errno_t result;
        size_t utf32LenProcessed;

        cstr_bool32 isLE;
        if ((flags & CSTR_FORBID_BOM) != 0) {
            return CSTR_EBOM;  /* Found a BOM, but it's forbidden. */
        }

        /* With this function, we need to use the endian defined by the BOM. */
        isLE = cstr_is_utf32_bom_le((const cstr_uint8*)pUTF32);
        
        pUTF32 += 1;    /* Skip past the BOM. */
        if (utf32Len != (size_t)-1) {
            utf32Len -= 1;
        }

        if (isLE) {
            result = cstr_utf32le_to_utf8(pUTF8, utf8Cap, pUTF8Len, pUTF32+1, utf32Len-1, &utf32LenProcessed, flags | CSTR_FORBID_BOM); /* <-- We already found a BOM, so we don't want to allow another occurance. */
        } else {
            result = cstr_utf32be_to_utf8(pUTF8, utf8Cap, pUTF8Len, pUTF32+1, utf32Len-1, &utf32LenProcessed, flags | CSTR_FORBID_BOM); /* <-- We already found a BOM, so we don't want to allow another occurance. */
        }

        if (pUTF32LenProcessed) {
            *pUTF32LenProcessed = utf32LenProcessed + 1;    /* +1 for the BOM. */
        }

        return result;
    }

    /* Getting here means there was no BOM, so assume native endian. */
    return cstr_utf32ne_to_utf8(pUTF8, utf8Cap, pUTF8Len, pUTF32+1, utf32Len-1, pUTF32LenProcessed, flags);
}



CSTR_API errno_t cstr_utf32_to_utf16_len_internal(size_t* pUTF16Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags, cstr_bool32 isLE)
{
    errno_t result = CSTR_SUCCESS;
    size_t utf16Len = 0;
    cstr_utf32 utf32;

    if (pUTF16Len != NULL) {
        *pUTF16Len = 0;
    }

    if (pUTF32LenProcessed != NULL) {
        *pUTF32LenProcessed = 0;
    }

    if (pUTF32 == NULL) {
        return EINVAL; /* Invalid input. */
    }

    if (utf32Len == 0 || pUTF32[0] == 0) {
        return CSTR_SUCCESS;  /* Empty input string. Length is always 0. */
    }

    /* Check for BOM. */
    if (cstr_has_utf32_bom((const cstr_uint8*)pUTF32, utf32Len)) {
        if ((flags & CSTR_FORBID_BOM) != 0) {
            return CSTR_EBOM;  /* Found a BOM, but it's forbidden. */
        }

        pUTF32 += 1;    /* Skip past the BOM. */
        if (utf32Len != (size_t)-1) {
            utf32Len -= 1;
        }
    }

    if (utf32Len == (size_t)-1) {
        /* Null terminated string. */
        const cstr_utf32* pUTF32Original = pUTF32;
        while (pUTF32[0] != 0) {
            if (isLE) {
                utf32 = cstr_le2host_32(pUTF32[0]);
            } else {
                utf32 = cstr_be2host_32(pUTF32[0]);
            }

            if (!cstr_is_valid_code_point(utf32)) {
                if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                    result = CSTR_ECODEPOINT;
                    break;
                } else {
                    utf32 = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                }
            }

            utf16Len += cstr_utf32_cp_to_utf16_len(utf32);
            pUTF32   += 1;
        }

        if (pUTF32LenProcessed != NULL) {
            *pUTF32LenProcessed = (pUTF32 - pUTF32Original);
        }
    } else {
        /* Fixed length string. */
        size_t iUTF32;
        for (iUTF32 = 0; iUTF32 < utf32Len; iUTF32 += 1) {
            if (isLE) {
                utf32 = cstr_le2host_32(pUTF32[iUTF32]);
            } else {
                utf32 = cstr_be2host_32(pUTF32[iUTF32]);
            }

            if (!cstr_is_valid_code_point(utf32)) {
                if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                    result = CSTR_ECODEPOINT;
                    break;
                } else {
                    utf32 = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                }
            }

            utf16Len += cstr_utf32_cp_to_utf16_len(utf32);
        }

        if (pUTF32LenProcessed != NULL) {
            *pUTF32LenProcessed = iUTF32;
        }
    }

    if (pUTF16Len != NULL) {
        *pUTF16Len = utf16Len;
    }

    return result;
}

errno_t cstr_utf32ne_to_utf16_len(size_t* pUTF16Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags)
{
    if (cstr_is_little_endian()) {
        return cstr_utf32le_to_utf16_len(pUTF16Len, pUTF32, utf32Len, pUTF32LenProcessed, flags);
    } else {
        return cstr_utf32be_to_utf16_len(pUTF16Len, pUTF32, utf32Len, pUTF32LenProcessed, flags);
    }
}

CSTR_API errno_t cstr_utf32le_to_utf16_len(size_t* pUTF16Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags)
{
    return cstr_utf32_to_utf16_len_internal(pUTF16Len, pUTF32, utf32Len, pUTF32LenProcessed, flags, CSTR_TRUE);
}

CSTR_API errno_t cstr_utf32be_to_utf16_len(size_t* pUTF16Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags)
{
    return cstr_utf32_to_utf16_len_internal(pUTF16Len, pUTF32, utf32Len, pUTF32LenProcessed, flags, CSTR_FALSE);
}

CSTR_API errno_t cstr_utf32_to_utf16_len(size_t* pUTF16Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags)
{
    if (pUTF16Len != NULL) {
        *pUTF16Len = 0;
    }

    /* Check for BOM. */
    if (cstr_has_utf32_bom((const cstr_uint8*)pUTF32, utf32Len)) {
        errno_t result;
        size_t utf32LenProcessed;

        cstr_bool32 isLE;
        if ((flags & CSTR_FORBID_BOM) != 0) {
            return CSTR_EBOM;  /* Found a BOM, but it's forbidden. */
        }

        /* With this function, we need to use the endian defined by the BOM. */
        isLE = cstr_is_utf32_bom_le((const cstr_uint8*)pUTF32);
        
        pUTF32 += 1;    /* Skip past the BOM. */
        if (utf32Len != (size_t)-1) {
            utf32Len -= 1;
        }

        if (isLE) {
            result = cstr_utf32le_to_utf16_len(pUTF16Len, pUTF32+1, utf32Len-1, &utf32LenProcessed, flags | CSTR_FORBID_BOM); /* <-- We already found a BOM, so we don't want to allow another occurance. */
        } else {
            result = cstr_utf32be_to_utf16_len(pUTF16Len, pUTF32+1, utf32Len-1, &utf32LenProcessed, flags | CSTR_FORBID_BOM); /* <-- We already found a BOM, so we don't want to allow another occurance. */
        }

        if (pUTF32LenProcessed) {
            *pUTF32LenProcessed = utf32LenProcessed + 1;    /* +1 for the BOM. */
        }

        return result;
    }

    /* Getting here means there was no BOM, so assume native endian. */
    return cstr_utf32ne_to_utf16_len(pUTF16Len, pUTF32, utf32Len, pUTF32LenProcessed, flags);
}


CSTR_API errno_t cstr_utf32_to_utf16_internal(cstr_utf16* pUTF16, size_t utf16Cap, size_t* pUTF16Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags, cstr_bool32 isLE)
{
    errno_t result = CSTR_SUCCESS;
    size_t utf16CapOriginal = utf16Cap;
    size_t utf16cpLen;  /* Code point length in UTF-8 code units. */
    cstr_utf32 utf32;

    if (pUTF16 == NULL) {
        return cstr_utf32_to_utf16_internal(pUTF16, utf16Cap, pUTF16Len, pUTF32, utf32Len, pUTF32LenProcessed, flags, isLE);
    }

    if (pUTF16Len != NULL) {
        *pUTF16Len = 0;
    }

    if (pUTF32LenProcessed != NULL) {
        *pUTF32LenProcessed = 0;
    }

    if (pUTF16 == NULL) {
        return EINVAL;
    }

    /* Check for BOM. */
    if (cstr_has_utf32_bom((const cstr_uint8*)pUTF32, utf32Len)) {
        if ((flags & CSTR_FORBID_BOM) != 0) {
            return CSTR_EBOM;  /* Found a BOM, but it's forbidden. */
        }

        pUTF32 += 1;    /* Skip past the BOM. */
        if (utf32Len != (size_t)-1) {
            utf32Len -= 1;
        }
    }

    if (utf32Len == (size_t)-1) {
        /* Null terminated string. */
        const cstr_utf32* pUTF32Original = pUTF32;
        while (pUTF32[0] != 0) {
            if (utf16Cap == 0) {
                result = ENOMEM;
                break;
            }

            if (isLE) {
                utf32 = cstr_le2host_32(pUTF32[0]);
            } else {
                utf32 = cstr_be2host_32(pUTF32[0]);
            }

            if (!cstr_is_valid_code_point(utf32)) {
                if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                    result = CSTR_ECODEPOINT;
                    break;
                } else {
                    utf32 = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                }
            }

            utf16cpLen = cstr_utf32_cp_to_utf16(utf32, pUTF16, utf16Cap);
            if (utf16cpLen == 0) {
                result = ENOMEM;    /* A return value of 0 at this point means there was not enough room in the output buffer. */
                break;
            }

            if (isLE) {
                if (utf16cpLen == 1) {
                    pUTF16[0] = cstr_host2le_16(pUTF16[0]);
                } else {
                    pUTF16[0] = cstr_host2le_16(pUTF16[0]);
                    pUTF16[1] = cstr_host2le_16(pUTF16[1]);
                }
            } else {
                if (utf16cpLen == 1) {
                    pUTF16[0] = cstr_host2be_16(pUTF16[0]);
                } else {
                    pUTF16[0] = cstr_host2be_16(pUTF16[0]);
                    pUTF16[1] = cstr_host2be_16(pUTF16[1]);
                }
            }

            pUTF16   += utf16cpLen;
            utf16Cap -= utf16cpLen;
            pUTF32   += 1;
        }

        if (pUTF32LenProcessed != NULL) {
            *pUTF32LenProcessed = (pUTF32 - pUTF32Original);
        }
    } else {
        /* Fixed length string. */
        size_t iUTF32;
        for (iUTF32 = 0; iUTF32 < utf32Len; iUTF32 += 1) {
            if (utf16Cap == 0) {
                result = ENOMEM;
                break;
            }

            if (isLE) {
                utf32 = cstr_le2host_32(pUTF32[iUTF32]);
            } else {
                utf32 = cstr_be2host_32(pUTF32[iUTF32]);
            }

            if (!cstr_is_valid_code_point(utf32)) {
                if ((flags & CSTR_ERROR_ON_INVALID_CODE_POINT) != 0) {
                    result = CSTR_ECODEPOINT;
                    break;
                } else {
                    utf32 = CSTR_UNICODE_REPLACEMENT_CODE_POINT;
                }
            }

            utf16cpLen = cstr_utf32_cp_to_utf16(utf32, pUTF16, utf16Cap);
            if (utf16cpLen == 0) {
                result = ENOMEM;    /* A return value of 0 at this point means there was not enough room in the output buffer. */
                break;
            }

            if (isLE) {
                if (utf16cpLen == 1) {
                    pUTF16[0] = cstr_host2le_16(pUTF16[0]);
                } else {
                    pUTF16[0] = cstr_host2le_16(pUTF16[0]);
                    pUTF16[1] = cstr_host2le_16(pUTF16[1]);
                }
            } else {
                if (utf16cpLen == 1) {
                    pUTF16[0] = cstr_host2be_16(pUTF16[0]);
                } else {
                    pUTF16[0] = cstr_host2be_16(pUTF16[0]);
                    pUTF16[1] = cstr_host2be_16(pUTF16[1]);
                }
            }

            pUTF16   += utf16cpLen;
            utf16Cap -= utf16cpLen;
        }

        if (pUTF32LenProcessed != NULL) {
            *pUTF32LenProcessed = iUTF32;
        }
    }

    /* Null terminate. */
    if (utf16Cap == 0) {
        result = ENOMEM;    /* Not enough room in the output buffer. */
    } else {
        pUTF16[0] = 0;
    }

    if (pUTF16Len != NULL) {
        *pUTF16Len = (utf16CapOriginal - utf16Cap);
    }

    return result;
}

CSTR_API errno_t cstr_utf32ne_to_utf16ne(cstr_utf16* pUTF16, size_t utf16Cap, size_t* pUTF16Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags)
{
    if (cstr_is_little_endian()) {
        return cstr_utf32le_to_utf16le(pUTF16, utf16Cap, pUTF16Len, pUTF32, utf32Len, pUTF32LenProcessed, flags);
    } else {
        return cstr_utf32be_to_utf16be(pUTF16, utf16Cap, pUTF16Len, pUTF32, utf32Len, pUTF32LenProcessed, flags);
    }
}

CSTR_API errno_t cstr_utf32le_to_utf16le(cstr_utf16* pUTF16, size_t utf16Cap, size_t* pUTF16Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags)
{
    return cstr_utf32_to_utf16_internal(pUTF16, utf16Cap, pUTF16Len, pUTF32, utf32Len, pUTF32LenProcessed, flags, CSTR_TRUE);
}

CSTR_API errno_t cstr_utf32be_to_utf16be(cstr_utf16* pUTF16, size_t utf16Cap, size_t* pUTF16Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags)
{
    return cstr_utf32_to_utf16_internal(pUTF16, utf16Cap, pUTF16Len, pUTF32, utf32Len, pUTF32LenProcessed, flags, CSTR_FALSE);
}

CSTR_API errno_t cstr_utf32_to_utf16(cstr_utf16* pUTF16, size_t utf16Cap, size_t* pUTF16Len, const cstr_utf32* pUTF32, size_t utf32Len, size_t* pUTF32LenProcessed, cstr_uint32 flags)
{
    if (pUTF16Len != NULL) {
        *pUTF16Len = 0;
    }

    /* Check for BOM. */
    if (cstr_has_utf32_bom((const cstr_uint8*)pUTF32, utf32Len)) {
        errno_t result;
        size_t utf32LenProcessed;

        cstr_bool32 isLE;
        if ((flags & CSTR_FORBID_BOM) != 0) {
            return CSTR_EBOM;  /* Found a BOM, but it's forbidden. */
        }

        /* With this function, we need to use the endian defined by the BOM. */
        isLE = cstr_is_utf32_bom_le((const cstr_uint8*)pUTF32);
        
        pUTF32 += 1;    /* Skip past the BOM. */
        if (utf32Len != (size_t)-1) {
            utf32Len -= 1;
        }

        if (isLE) {
            result = cstr_utf32le_to_utf16le(pUTF16, utf16Cap, pUTF16Len, pUTF32+1, utf32Len-1, &utf32LenProcessed, flags | CSTR_FORBID_BOM); /* <-- We already found a BOM, so we don't want to allow another occurance. */
        } else {
            result = cstr_utf32be_to_utf16be(pUTF16, utf16Cap, pUTF16Len, pUTF32+1, utf32Len-1, &utf32LenProcessed, flags | CSTR_FORBID_BOM); /* <-- We already found a BOM, so we don't want to allow another occurance. */
        }

        if (pUTF32LenProcessed) {
            *pUTF32LenProcessed = utf32LenProcessed + 1;    /* +1 for the BOM. */
        }

        return result;
    }

    /* Getting here means there was no BOM, so assume native endian. */
    return cstr_utf32ne_to_utf16ne(pUTF16, utf16Cap, pUTF16Len, pUTF32+1, utf32Len-1, pUTF32LenProcessed, flags);
}



/**************************************************************************************************************************************************************

Utilities

**************************************************************************************************************************************************************/
CSTR_API cstr_bool32 cstr_utf32_is_null_or_whitespace(const cstr_utf32* pUTF32, size_t utf32Len)
{
    if (pUTF32 == NULL) {
        return CSTR_TRUE;
    }

    while (pUTF32[0] != 0 && utf32Len > 0) {
        cstr_utf32 cp = pUTF32[0];

        pUTF32   += 1;
        utf32Len -= 1;
        
        if (cp >= 0x09 && cp <= 0x0D) {
            continue;
        }

        if (cp >= 0x2000 && cp <= 0x200A) {
            continue;
        }

        switch (cp) {
            case 0x0020:
            case 0x0085:
            case 0x00A0:
            case 0x1680:
            case 0x2028:
            case 0x2029:
            case 0x202F:
            case 0x205F:
            case 0x3000:
                continue;

            default:
                return CSTR_FALSE;
        }
    }

    /* Getting here means we reached the end of the string without finding anything other than whitespace which means the string is entire whitespace. */
    return CSTR_TRUE;
}

CSTR_API cstr_bool32 cstr_utf32_is_newline(cstr_utf32 utf32)
{
    if (utf32 >= 0x0A && utf32 <= 0x0D) {
        return CSTR_TRUE;
    }

    if (utf32 == 0x85) {
        return CSTR_TRUE;
    }

    if (utf32 >= 0x2028 && utf32 <= 0x2029) {
        return CSTR_TRUE;
    }

    return CSTR_FALSE;
}

CSTR_API cstr_bool32 cstr_utf8_is_null_or_whitespace(const cstr_utf8* pUTF8, size_t utf8Len)
{
    if (pUTF8 == NULL) {
        return CSTR_TRUE;
    }

    /* This could be faster, but it's practical. */
    while (pUTF8[0] != '\0' && utf8Len > 0) {
        cstr_utf32 utf32;
        size_t utf8Processed;
        int err;

        /* We expect ENOMEM to be returned, but we should still have a valid utf32 character. */
        err = cstr_utf8_to_utf32(&utf32, 1, NULL, pUTF8, utf8Len, &utf8Processed, 0);
        if (err != 0 && err != ENOMEM) {
            break;
        }

        if (utf8Processed == 0) {
            break;
        }

        if (cstr_utf32_is_null_or_whitespace(&utf32, 1) == CSTR_FALSE) {
            return CSTR_FALSE;
        }

        pUTF8   += utf8Processed;
        utf8Len -= utf8Processed;
    }

    return CSTR_TRUE;
}


CSTR_API size_t cstr_utf8_ltrim_offset(const cstr_utf8* pUTF8, size_t utf8Len)
{
    size_t utf8RunningOffset = 0;

    if (pUTF8 == NULL) {
        return cstr_npos;
    }

    while (pUTF8[0] != '\0' && utf8Len > 0) {
        cstr_utf32 utf32;
        size_t utf8Processed;
        int err;

        err = cstr_utf8_to_utf32(&utf32, 1, NULL, pUTF8, utf8Len, &utf8Processed, 0);
        if (err != 0 && err != ENOMEM) {
            break;
        }

        if (utf8Processed == 0) {
            break;
        }

        if (cstr_utf32_is_null_or_whitespace(&utf32, 1) == CSTR_FALSE) {
            break;
        }

        utf8RunningOffset += utf8Processed;
        pUTF8             += utf8Processed;
        utf8Len           -= utf8Processed;
    }

    return utf8RunningOffset;
}

CSTR_API size_t cstr_utf8_rtrim_offset(const cstr_utf8* pUTF8, size_t utf8Len)
{
    size_t utf8RunningOffset = 0;
    size_t utf8LastNonWhitespaceOffset = utf8Len;

    if (pUTF8 == NULL) {
        return cstr_npos;
    }

    for (;;) {
        cstr_utf32 utf32;
        size_t utf8Processed;
        int err;

        err = cstr_utf8_to_utf32(&utf32, 1, NULL, pUTF8, utf8Len, &utf8Processed, 0);
        if (err != 0 && err != ENOMEM) {
            break;
        }

        if (utf8Processed == 0) {
            break;
        }

        utf8RunningOffset += utf8Processed;

        if (cstr_utf32_is_null_or_whitespace(&utf32, 1) == CSTR_FALSE) {
            utf8LastNonWhitespaceOffset = utf8RunningOffset;
        }

        pUTF8   += utf8Processed;
        utf8Len -= utf8Processed;
    }

    return utf8LastNonWhitespaceOffset;
}

CSTR_API size_t cstr_utf8_next_line(const cstr_utf8* pUTF8, size_t utf8Len, size_t* pThisLineLen)
{
    size_t thisLen = 0;
    size_t nextBeg = 0;

    if (pThisLineLen == NULL) {
        *pThisLineLen = 0;
    }

    if (pUTF8 == NULL) {
        return cstr_npos;
    }

    /* This could be faster, but it's practical. */
    while (pUTF8[0] != '\0' && utf8Len > 0) {
        cstr_utf32 utf32;
        size_t utf8Processed;
        int err;

        /* We expect ENOMEM to be returned, but we should still have a valid utf32 character. */
        err = cstr_utf8_to_utf32(&utf32, 1, NULL, pUTF8, utf8Len, &utf8Processed, 0);
        if (err != 0 && err != ENOMEM) {
            break;
        }

        if (utf8Processed == 0) {
            break;
        }

        nextBeg += utf8Processed;

        if (cstr_utf32_is_newline(utf32) == CSTR_TRUE) {
            /* Special case for \r\n. This needs to be treated as one line. The \r by itself should also be treated as a new line, however. */
            if (utf8Len + utf8Processed > 0 && pUTF8[utf8Processed] == '\n') {
                nextBeg += 1;
            }

            break;
        }

        CSTR_ASSERT(utf8Len >= utf8Processed);  /* If there wasn't enough data in the UTF-8 string, cstr_utf8_to_utf32() should have failed. */

        thisLen += utf8Processed;
        pUTF8   += utf8Processed;
        utf8Len -= utf8Processed;
    }

    if (pThisLineLen != NULL) {
        *pThisLineLen = thisLen;
    }

    return nextBeg;
}

#endif  /* libcstr_c */
#endif  /* LIBCSTR_IMPLEMENTATION */

/*
This software is available as a choice of the following licenses. Choose
whichever you prefer.

===============================================================================
ALTERNATIVE 1 - Public Domain (www.unlicense.org)
===============================================================================
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>

===============================================================================
ALTERNATIVE 2 - MIT No Attribution
===============================================================================
Copyright 2020 David Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
