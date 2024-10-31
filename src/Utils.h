#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string_view>

#ifdef WIN32
#define DEBUG_BREAK() __debugbreak()
#else
#include <signal.h>
#define DEBUG_BREAK() raise(SIGTRAP)
#endif
#include <signal.h>

#define ERROR_EXIT(...) { fprintf(stderr, "ERORR: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "!\n"); DEBUG_BREAK(); exit(1); }
#define ASSERT(x) { if (!(x)) { fprintf(stderr, "ERROR assert failed: " __FILE__ " line %d!\n", __LINE__); DEBUG_BREAK(); exit(1); } }

namespace utils
{
    inline bool starts_with(std::string_view str, std::string_view check)
    {
        if (str.length() < check.length())
            return false;
        return strncmp(str.data(), check.data(), check.size()) == 0;
    }

    inline bool ends_with(std::string_view str, std::string_view check)
    {
        if (str.length() < check.length())
            return false;
        const char* str_end = str.data() + (str.length() - check.length());
        return strncmp(str_end, check.data(), check.size()) == 0;
    }
}
