#ifndef _____PRINTLN_____
#define _____PRINTLN_____
#include<fmt/format.h>
#include<cstdio>
using fmt::format;

template <typename S, typename... Args>
inline void println (FILE* out, const S& format, Args&&... args) {
fmt::print(format, args...);
fputs("\n", out);
fflush(out);
}

template <typename S, typename... Args>
inline void println (const S& format, Args&&... args) {
println(stdout, format, args...);
}

#endif
