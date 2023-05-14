#ifndef SHARED_HXX
#define SHARED_HXX

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

#include <iostream>
#include <string>
#include <fstream>

static constexpr char REFRESH_FILE[] = "refresh.txt";

[[noreturn]]
static inline void pause_then_exit(int code) {
    std::cin.get();
    std::exit(code);
}

#endif