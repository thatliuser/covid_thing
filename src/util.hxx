#ifndef UTIL_HXX
#define UTIL_HXX

#include "shared.hxx"
#include <stringapiset.h>

namespace util {
    template <class... Args>
    inline void print(Args... args) {
        (std::cout << ... << args);
    }

    template <class... Args>
    inline void println(Args... args) {
        print(args..., '\n');
    }

    inline std::string narrow(const wchar_t* wide) {
        int length = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
        char* buf = new char[length];
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, buf, length, nullptr, nullptr);
        std::string result = buf;
        delete[] buf;
        return result;
    }

    inline std::wstring widen(const char* narrow) {
        int length = MultiByteToWideChar(CP_UTF8, 0, narrow, -1, nullptr, 0);
        wchar_t* buf = new wchar_t[length];
        MultiByteToWideChar(CP_UTF8, 0, narrow, -1, buf, length);
        std::wstring result = buf;
        delete[] buf;
        return result;
    }

    template <class T>
    class Slice {
    public:
        T* pointer;
        size_t length;

    public:
        constexpr void set(T* pointer, size_t length) {
            this->pointer = pointer;
            this->length = length;
        }

        template <size_t length>
        constexpr void set(T(&arr)[length]) {
            this->set(arr, length);
        }

        constexpr Slice(T* pointer, size_t length) 
            : pointer(pointer), length(length) {}

        template <size_t length>
        constexpr Slice(T(&arr)[length]) 
            : Slice(arr, length) {}

        constexpr Slice()
            : Slice(nullptr, 0) {}
    };
}

#endif