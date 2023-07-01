@echo off
mingw32-make mode=release 2>&1 | unix2dos
mingw32-make doc mode=release 2>&1 | unix2dos