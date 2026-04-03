@echo off
set "PATH=C:\msys64\ucrt64\bin;%PATH%"
echo Using GCC:
g++ --version
echo Compiling Mafia game (all sources)...
windres mafia.rc -O coff -o mafia.res.o
g++ -std=c++20 -O2 *.cpp mafia.res.o -o mafia.exe -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
if errorlevel 1 (
	echo Build failed.
	exit /b 1
)
echo Build succeeded: mafia.exe
