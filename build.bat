@echo OFF

set MINGW_BIN_DIR=C:\TDM-GCC-32\bin



set Path=%MINGW_BIN_DIR%;%Path%

@echo ON

g++ -Wall -O2 -c main.cpp -o main.o
g++ main.o -o calibre4mhl.exe -s