#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build dist
x86_64-w64-mingw32-windres -I Source Source/ImageToolkit.rc -O coff -o build/resources.o
x86_64-w64-mingw32-gcc -std=c11 -O2 -Wall -Wextra -Werror -municode -mwindows -static-libgcc Native/main.c Native/windows_entry.c build/resources.o -o dist/ImageToolkit.exe -lgdiplus -lcomdlg32 -lshell32 -lole32 -lcomctl32 -lgdi32 -luser32
