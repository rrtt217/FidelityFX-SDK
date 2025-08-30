#pragma once
#ifndef _MSC_VER
    #include <cwchar>
    #include <cstring>
    #include <iterator>

    // 3-parameter version - Complete safety checking
    // MSVC compatible wcscpy_s function with buffer size validation
    inline int wcscpy_s(wchar_t* dest, size_t destsz, const wchar_t* src) {
        // Validate input parameters
        if (!dest || !src || destsz == 0) return 1;
        
        // Copy characters while checking buffer bounds
        size_t i = 0;
        for (; i + 1 < destsz && src[i] != L'\0'; ++i) {
            dest[i] = src[i];
        }
        
        // Ensure null termination
        if (i < destsz) {
            dest[i] = L'\0';  // Normal termination
        } else {
            dest[destsz - 1] = L'\0';  // Forced termination due to buffer limit
            return 1; // Truncation error
        }
        return 0; // Success
    }

    // 2-parameter version - Assume destination buffer is large enough
    // Compatibility version for cases where buffer size is not specified
    inline int wcscpy_s(wchar_t* dest, const wchar_t* src) {
        // Validate input parameters
        if (!dest || !src) return 1;
        
        // Simple copy without buffer size checking (potentially unsafe)
        while ((*dest++ = *src++) != L'\0') {}
        return 0; // Success
    }

    // Safe strcpy_s implementation with buffer size validation
    inline int strcpy_s(char* dest, size_t destsz, const char* src) {
        // Validate input parameters
        if (!dest || !src || destsz == 0) return 1;
        
        // Copy characters while checking buffer bounds
        size_t i = 0;
        for (; i + 1 < destsz && src[i] != '\0'; ++i) {
            dest[i] = src[i];
        }
        
        // Ensure null termination
        if (i < destsz) {
            dest[i] = '\0';  // Normal termination
        } else {
            if (destsz > 0) {
                dest[destsz - 1] = '\0';  // Forced termination
            }
            return 1; // Truncation error
        }
        return 0; // Success
    }

    // Simplified 2-parameter version of strcpy_s
    // Compatibility version without buffer size checking
    inline int strcpy_s(char* dest, const char* src) {
        // Validate input parameters
        if (!dest || !src) return 1;
        
        // Simple copy without buffer size checking (potentially unsafe)
        while ((*dest++ = *src++) != '\0') {}
        return 0; // Success
    }

    // _countof macro - Calculate array element count
    // Same as wsl stub in directx-headers
    #ifndef _countof
        #define _countof(a) (sizeof(a) / sizeof(*(a)))
    #endif

#endif