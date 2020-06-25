This is a simple string library for C. Notable features are a dynamic string API and a suite of UTF-8, UTF-16 and
UTF-32 conversion routines. Unicode conversion routines are implemented for all combinations, with the exception of
converting between different endians. Swapping endians can be done separately.

I've built this to service my own requirements, but if there's something you might find useful let me know and I'll
consider adding support (no guarantees). I have plans for the following APIs:

    cstr_sprintf()
    cstr_vsprintf()
    cstr_snprintf()
    cstr_vsnprintf()
    cstr_strcasecmp()
    cstr_strncasecmp()
    
Documentation can be found in libcstr.h. It's a single file library, but there's a libcstr.c file for those who
prefer separate .h and .c files.

I don't maintain version numbers nor release notes for this project. Use the Git log to see what's changed. I make
no guarantees on API stability and may change anything at any time, but I try to keep breaking changes to a minimum.

Public domain or MIT-0, whichever you prefer.


Function List
=============
Standard Library Replacements
-----------------------------
    cstr_strlen
    cstr_strcpy
    cstr_strcpy_s
    cstr_strncpy_s
    cstr_strcat_s
    cstr_strncat_s

Dynamic Strings
---------------
    cstr_alloc
    cstr_free
    cstr_newn
    cstr_new
    cstr_newv
    cstr_newf
    cstr_setn
    cstr_set
    cstr_catn
    cstr_cat
    cstr_len
    cstr_cap
    cstr_find
    cstr_find_last
    cstr_substr_tagged
    cstr_new_substr_tagged
    cstr_replace_range
    cstr_replace_range_tagged

Unicode Conversion
------------------
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
    
    cstr_swap_endian_utf16
    cstr_swap_endian_utf32
    cstr_has_utf8_bom
    cstr_has_utf16_bom
    cstr_has_utf32_bom
    cstr_is_utf16_bom_le
    cstr_is_utf16_bom_be
    cstr_is_utf32_bom_le
    cstr_is_utf32_bom_be
