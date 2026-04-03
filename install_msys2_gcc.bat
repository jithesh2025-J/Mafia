@echo off
echo Installing/Updating MSYS2 GCC...

REM Launch MSYS2 UCRT64 for pacman (non-interactive)
if exist "C:\msys64\ucrt64.exe" (
    echo Running pacman -Syu...
    C:\msys64\ucrt64.exe -defterm -c "pacman -Syu --noconfirm &amp;&amp; pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-pkg-config base-devel"
    echo MSYS2 GCC installation complete.
) else (
    echo MSYS2 not found at C:\msys64. Download from msys2.org first.
)

REM Update PATH to prioritize MSYS2 (requires restart)
setx /M PATH "C:\msys64\ucrt64\bin;%PATH%" >nul
echo PATH updated. **Restart VSCode/terminal**. Then test: g++ --version

pause

