@echo off
rem
rem =================================
rem
rem RELEASE VERSION 1.1
rem
rem GCC (MinGW-w64) UEFI Bootloader Windows Cleanup Script
rem
rem by KNNSpeed
rem
rem =================================
rem
rem The above @echo off disables displaying all of the code you see here in the
rem command line
rem

echo.
echo Running cleanup procedures; press CTRL+C to exit now. Otherwise...
pause

rem
rem Move into the Backend folder, where all the magic happens
rem

setlocal

set CurDir=%CD%

cd ../Backend

rem
rem Delete generated files
rem

del BOOTX64.EFI
del output.map
del objects.list

@echo on
FOR /F "tokens=*" %%f IN ('type "%CurDir%\c_files_windows.txt"') DO (del "%%~df%%~pf%%~nf.o" & del "%%~df%%~pf%%~nf.d" & del "%%~df%%~pf%%~nf.out")
@echo off

rem
rem Move into startup folder
rem

cd "%CurDir%\startup"

rem
rem Delete compiled object files
rem

del *.o
del *.d
del *.out

rem
rem Move into user source directory
rem

cd "%CurDir%\src"

rem
rem Delete compiled object files
rem

del *.o
del *.d
del *.out

rem
rem Return to folder started from
rem

endlocal

rem
rem Display completion message and exit
rem

echo.
echo Done! All clean.
echo.
