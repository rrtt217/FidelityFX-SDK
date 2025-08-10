#pragma once
#ifndef _MSC_VER
    // msvc-compatible wcscpy_s function
    inline int wcscpy_s(wchar_t* dest, size_t destsz, const wchar_t* src) {
    if (!dest || !src || destsz == 0) return 1;
    size_t i = 0;
    for (; i + 1 < destsz && src[i] != L'\0'; ++i) {
        dest[i] = src[i];
    }
    if (i < destsz)
        dest[i] = L'\0';
    else
        dest[destsz - 1] = L'\0';
    if (src[i] != L'\0') return 1;
    return 0;
    }

    // msvc-compatible wcscpy_s function without size check
    inline int wcscpy_s(wchar_t* dest, const wchar_t* src) {
    if (!dest || !src) return 1;
    while ((*dest++ = *src++) != L'\0') {}
    return 0;
    }
    // msvc-compatible strcpy_s function
    #define strcpy_s(dest, size, src) std::strncpy(dest, src, size)
    // msvc-compatible _countof macro, same as wsl stub in directx-headers
    #ifndef _countof
        #define _countof(a) (sizeof(a) / sizeof(*(a)))
    #endif
    // includes that needed in gcc,etc.
    #include <cwchar>
    #include <iterator>
    #include <cstring>
#endif