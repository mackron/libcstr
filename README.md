This is a simple string library for C. Notable features are a dynamic string API and a suite of UTF-8, UTF-16 and
UTF-32 conversion routines. Unicode conversion routines are implemented for all combinations, with the exception of
converting between different endians. Swapping endians can be done separately.

I've built this to service my own requirements, but if there's something you might find useful let me know and I'll
consider adding support (no guarantees). I have plans for the following APIs:

    utf8_sprintf()
    utf8_vsprintf()
    utf8_snprintf()
    utf8_vsnprintf()
    utf8_strcasecmp()
    utf8_strncasecmp()
    
Documentation can be found in libcstr.h. It's a single file library, but there's a libcstr.c file for those who
prefer separate .h and .c files.

I don't maintain version numbers nor release notes for this project. Use the Git log to see what's changed. I make
no guarantees on API stability and may change anything at any time, but I try to keep breaking changes to a minimum.

Public domain or MIT-0, whichever you prefer.


Function List
=============
Standard Library Replacements
-----------------------------
    utf8_strlen
    utf8_strcpy
    utf8_strcpy_s
    utf8_strncpy_s
    utf8_strcat_s
    utf8_strncat_s
    utf8_atoi_s

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
    utf8_to_utf16ne
    utf8_to_utf16le
    utf8_to_utf16be
    utf8_to_utf16
    utf8_to_utf32ne
    utf8_to_utf32le
    utf8_to_utf32be
    utf8_to_utf32
    utf16ne_to_utf8
    utf16le_to_utf8
    utf16be_to_utf8
    utf16_to_utf8
    utf16ne_to_utf32ne
    utf16le_to_utf32le
    utf16be_to_utf32be
    utf16_to_utf32
    utf32ne_to_utf8
    utf32le_to_utf8
    utf32be_to_utf8
    utf32_to_utf8
    utf32ne_to_utf16ne
    utf32le_to_utf16le
    utf32be_to_utf16be
    utf32_to_utf16
    
    utf8_to_utf16_len
    utf8_to_utf16ne_len
    utf8_to_utf16le_len
    utf8_to_utf16be_len
    utf8_to_utf32_len
    utf8_to_utf32ne_len
    utf8_to_utf32le_len
    utf8_to_utf32be_len
    utf16ne_to_utf8_len
    utf16le_to_utf8_len
    utf16be_to_utf8_len
    utf16_to_utf8_len
    utf16ne_to_utf32_len
    utf16le_to_utf32_len
    utf16be_to_utf32_len
    utf16ne_to_utf32ne_len
    utf16le_to_utf32le_len
    utf16be_to_utf32be_len
    utf16_to_utf32_len
    utf32ne_to_utf8_len
    utf32le_to_utf8_len
    utf32be_to_utf8_len
    utf32_to_utf8_len
    utf32ne_to_utf16_len
    utf32le_to_utf16_len
    utf32be_to_utf16_len
    utf32ne_to_utf16ne_len
    utf32le_to_utf16le_len
    utf32be_to_utf16be_len
    utf32_to_utf16_len
    
    utf16_swap_endian
    utf32_swap_endian
    utf8_has_bom
    utf16_has_bom
    utf32_has_bom
    utf16_is_bom_le
    utf16_is_bom_be
    utf32_is_bom_le
    utf32_is_bom_be
