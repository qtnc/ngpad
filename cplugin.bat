@echo off
g++ -s -O3 -std=gnu++17 -shared -o htmltools.dll src\plugins\html\*.cpp -lfmt -lwxbase31u -lwxmsw31u_core 