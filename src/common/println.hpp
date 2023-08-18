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

template <typename S, typename... Args>
inline void print (FILE* out, const S& format, Args&&... args) {
fmt::print(format, args...);
fflush(out);
}

template <typename S, typename... Args>
inline void print (const S& format, Args&&... args) {
print(stdout, format, args...);
}

#endif
