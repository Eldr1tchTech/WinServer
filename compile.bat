@echo off
setlocal enabledelayedexpansion

SET SOURCE_DIR=server
SET OUTPUT_DIR=bin

SET OUTPUT_FILE=server
SET OUTPUT_TYPE=exe

SET COMPILER_FLAGS=-g -Wall -Werror
SET INCLUDE_FLAGS=-I%SOURCE_DIR% -I%SOURCE_DIR%\networking
SET LINKER_FLAGS=-lws2_32
SET DEFINES=-D_CRT_SECURE_NO_WARNINGS

echo Compiling project with clang...

if not exist %OUTPUT_DIR% mkdir %OUTPUT_DIR%

:: Build list of all source files recursively
set SOURCES=
for /R %SOURCE_DIR% %%f in (*.c) do (
    set SOURCES=!SOURCES! %%f
)

:: Compile everything
clang !SOURCES! -o %OUTPUT_DIR%\%OUTPUT_FILE%.%OUTPUT_TYPE% %DEFINES% %COMPILER_FLAGS% %INCLUDE_FLAGS% %LINKER_FLAGS%

endlocal