#ifdef __WIN32
#define UNICODE
#include<windows.h>

unsigned long long GetUSTick () {
LARGE_INTEGER l;
QueryPerformanceCounter(&l);
return l.QuadPart;
}

unsigned long long GetUSElapsedTime (unsigned long long start, unsigned long long end) {
static unsigned long long freq = 0;
if (!freq) {
LARGE_INTEGER l;
QueryPerformanceFrequency(&l);
freq = l.QuadPart;
}
return (end - start) * 1000000 / freq;
}


#endif
