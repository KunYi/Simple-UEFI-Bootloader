@echo off
:rerun_script
rem
rem =================================
rem
rem VERSION 1.1
rem
rem GCC (MinGW-w64) UEFI Bootloader Windows Compile Script
rem
rem by KNNSpeed
rem
rem =================================
rem
rem The above @echo off disables displaying all of the code you see here in the
rem command line
rem

rem
rem Allow local variables to be created (so that this script doesn't mess with
rem system variables). Also, ENABLEDELAYEDEXPANSION allows local variables to
rem be modified
rem

setlocal ENABLEDELAYEDEXPANSION

rem
rem Globally enable or disable display of commands that are run at compile time
rem

set echo_stat=off

rem
rem CurDir is DOS path
rem CurDir2 is DOS path with /
rem CurDir3 is Unix path with \ before spaces
rem
rem These are needed because GCC is a native UNIX executable, and expects UNIX-
rem like syntax in certain places. Without these, you couldn't use spaces in
rem folder names!
rem

set CurDir=%CD%
set CurDir2=%CurDir:\=/%
set CurDir3=%CurDir2: =\ %
set GCC_FOLDER_NAME=mingw64

rem
rem Move into the Backend folder, where all the magic happens
rem

cd ../Backend

rem
rem First things first, delete the objects list to rebuild it later
rem

del objects.list

rem
rem Need to set path for various DLLs in the GCC folder, otherwise compile will
rem fail. Due to the above setlocal, this is not a persistent PATH edit; it
rem will only apply to this script.
rem
rem Also Need to tell GCC where to find as and ld
rem

set PATH=%CD%\%GCC_FOLDER_NAME%\bin;%PATH%

rem
rem For debugging this script, this echo helps ensure the path is set correctly
rem

rem echo !PATH!
rem pause

rem
rem Create the HFILES variable, which contains the massive set of includes (-I)
rem needed by GCC.
rem
rem Two of the include folders are always included, and they
rem are %CurDir%/inc/ (the user-header directory) and %CurDir%/startup/
rem

set HFILES="%CurDir%\inc\" -I"%CurDir%\startup\"

rem
rem Loop through the h_files.txt file and turn each include directory into -I strings
rem

FOR /F "tokens=*" %%h IN ('type "%CurDir%\h_files.txt"') DO set HFILES=!HFILES! -I"%%h"

rem
rem Windows uses backslashes. GCC and *nix-based things expect forward slashes.
rem This converts backslashes into forward slashes on Windows.
rem

set HFILES=%HFILES:\=/%

rem
rem This echo is useful for debugging this script, namely to make sure you
rem aren't missing any include directories.
rem

rem echo !HFILES!
rem pause

rem
rem Loop through and compile the backend .c files, which are listed in c_files_windows.txt
rem

@echo %echo_stat%
FOR /F "tokens=*" %%f IN ('type "%CurDir%\c_files_windows.txt"') DO "%GCC_FOLDER_NAME%\bin\gcc.exe" -ffreestanding -fshort-wchar -fno-stack-protector -fno-stack-check -fno-strict-aliasing -mno-stack-arg-probe -fno-merge-all-constants -m64 -mno-red-zone -DGNU_EFI_USE_MS_ABI -maccumulate-outgoing-args --std=c11 -I!HFILES! -Og -g3 -Wall -Wextra -Wdouble-promotion -fmessage-length=0 -c -MMD -MP -Wa,-adhln="%%~df%%~pf%%~nf.out" -MF"%%~df%%~pf%%~nf.d" -MT"%%~df%%~pf%%~nf.o" -o "%%~df%%~pf%%~nf.o" "%%~ff"
@echo off

rem
rem Compile the .c files in the startup folder (if any exist)
rem

@echo %echo_stat%
FOR %%f IN ("%CurDir2%/startup/*.c") DO "%GCC_FOLDER_NAME%\bin\gcc.exe" -ffreestanding -fshort-wchar -fno-stack-protector -fno-stack-check -fno-strict-aliasing -fno-merge-all-constants -mno-stack-arg-probe -m64 -mno-red-zone -DGNU_EFI_USE_MS_ABI -maccumulate-outgoing-args --std=c11 -I!HFILES! -Og -g3 -Wall -Wextra -Wdouble-promotion -fmessage-length=0 -c -MMD -MP -Wa,-adhln="%CurDir2%/startup/%%~nf.out" -MF"%CurDir2%/startup/%%~nf.d" -MT"%CurDir2%/startup/%%~nf.o" -o "%CurDir2%/startup/%%~nf.o" "%CurDir2%/startup/%%~nf.c"
@echo off

rem
rem Compile the .s files in the startup folder (Any assembly files needed to
rem initialize the system)
rem

@echo %echo_stat%
FOR %%f IN ("%CurDir2%/startup/*.s") DO "%GCC_FOLDER_NAME%\bin\gcc.exe" -ffreestanding -fshort-wchar -fno-stack-protector -fno-stack-check -fno-strict-aliasing -fno-merge-all-constants -mno-stack-arg-probe -m64 -mno-red-zone -DGNU_EFI_USE_MS_ABI -maccumulate-outgoing-args --std=c11 -I"%CurDir2%/inc/" -g -o "%CurDir2%/startup/%%~nf.o" "%CurDir2%/startup/%%~nf.s"
@echo off

rem
rem Compile user .c files
rem

@echo on
FOR %%f IN ("%CurDir2%/src/*.c") DO "%GCC_FOLDER_NAME%\bin\gcc.exe" -ffreestanding -fshort-wchar -fno-stack-protector -fno-stack-check -fno-strict-aliasing -fno-merge-all-constants -mno-stack-arg-probe -m64 -mno-red-zone -DGNU_EFI_USE_MS_ABI -maccumulate-outgoing-args --std=c11 -I!HFILES! -Og -g3 -Wall -Wextra -Wdouble-promotion -fmessage-length=0 -c -MMD -MP -Wa,-adhln="%CurDir2%/src/%%~nf.out" -MF"%CurDir2%/src/%%~nf.d" -MT"%CurDir2%/src/%%~nf.o" -o "%CurDir2%/src/%%~nf.o" "%CurDir2%/src/%%~nf.c"
@echo off

rem
rem Create OBJECTS variable, whose sole purpose is to allow conversion of
rem backslashes into forward slashes in Windows
rem

set OBJECTS=

rem
rem Create the objects.list file, which contains properly-formatted (i.e. has
rem forward slashes) locations of compiled Backend .o files
rem

FOR /F "tokens=*" %%f IN ('type "%CurDir%\c_files_windows.txt"') DO (set OBJECTS="%%~df%%~pf%%~nf.o" & set OBJECTS=!OBJECTS:\=/! & set OBJECTS=!OBJECTS: =\ ! & set OBJECTS=!OBJECTS:"\ \ ="! & echo !OBJECTS! >> objects.list)

rem
rem Add compiled .o files from the startup directory to objects.list
rem

FOR %%f IN ("%CurDir2%/startup/*.o") DO echo "%CurDir3%/startup/%%~nxf" >> objects.list

rem
rem Add compiled user .o files to objects.list
rem

FOR %%f IN ("%CurDir2%/src/*.o") DO echo "%CurDir3%/src/%%~nxf" >> objects.list

rem
rem Link the object files using all the objects in objects.list to generate the
rem output binary, which is called "BOOTX64.EFI"
rem

@echo on
"%GCC_FOLDER_NAME%\bin\gcc.exe" -nostdlib -Wl,--warn-common -Wl,--no-undefined -Wl,-dll -s -shared -Wl,--subsystem,10 -e efi_main -Wl,-Map=output.map -o "BOOTX64.EFI" @"objects.list"
@echo off
rem Remove -s in the above command to keep debug symbols in the output binary.

rem
rem Output the program size
rem

echo.
echo Generating binary and Printing size information:
echo.
rem "%GCC_FOLDER_NAME%\bin\objcopy.exe" -O binary "program.exe" "program.bin"
"%GCC_FOLDER_NAME%\bin\size.exe" "BOOTX64.EFI"
echo.

set /P UPL="Cleanup, recompile, or done? [c for cleanup, r for recompile, any other key for done] "

echo.
echo **********************************************************
echo.

if /I "%UPL%"=="C" endlocal & .\Cleanup.bat
if /I "%UPL%"=="R" endlocal & goto rerun_script

rem
rem Return to the folder started from and exit
rem
rem No more need for local variables
rem

endlocal
