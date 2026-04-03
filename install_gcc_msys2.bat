@echo off
REM Check if MSYS2 exists
if exist "C:\msys64\ucrt64\bin\g++.exe" (
    echo MSYS2 GCC already exists. Updating...
&amp;&amp;
) else (
    echo MSYS2 not found. Please install first.
    pause
    exit /b 1
)

REM Add/prioritize MSYS2 PATH (User PATH)
set "MSYSBIN=C:\msys64\ucrt64\bin"
powershell -Command "$env:PATH = '%MSYSBIN%';$env:PATH -split ';' | Sort-Object -Unique | ForEach {"$_.Trim('""')"} | Out-File $env:TEMP\path_fixed.txt; Get-Content $env:TEMP\path_fixed.txt | Out-File -Append '%USERPROFILE%\.vscode\path_backup.txt'"
setx PATH "%MSYSBIN%;%PATH%" /M

echo GCC install complete. Restart VSCode and run: g++ --version
pause
