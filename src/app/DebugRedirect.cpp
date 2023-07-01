#include<cstdio>
using namespace std;

void __attribute__((constructor)) initDebugRedirect (void) {
freopen("debug.txt", "w", stdout);
freopen("debug.txt", "a", stderr);
}
