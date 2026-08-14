#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(BLACKBOX_EXPORTS)
        #define BLACKBOX_API __declspec(dllexport)
    #else
        #define BLACKBOX_API __declspec(dllimport)
    #endif
#else
    #if __GNUC__ >= 4
        #define BLACKBOX_API __attribute__((visibility("default")))
    #else
        #define BLACKBOX_API
    #endif
#endif