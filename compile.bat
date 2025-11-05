@echo off
echo Compiling Employee Record System...

:: First, try to compile with just the basic options
gcc EmployeeRecordSystem.c -o EmployeeRecordSystem.exe -w

:: Check if compilation was successful
if %errorlevel% neq 0 (
    echo Compilation failed! Trying alternative compilation...
    :: Try alternate compilation with additional libraries
    gcc EmployeeRecordSystem.c -o EmployeeRecordSystem.exe -w -lm
)

:: Check final result
if %errorlevel% equ 0 (
    echo Compilation successful!
    echo Running the program...
    start cmd /k EmployeeRecordSystem.exe
) else (
    echo Compilation failed. Please check the error messages above.
    pause
)