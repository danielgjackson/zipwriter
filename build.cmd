@ECHO OFF
SETLOCAL EnableDelayedExpansion
CD /D %~dp0

set ARCH=x86

SET VCVARSALL=
FOR %%f IN (70 71 80 90 100 110 120 130 140) DO IF EXIST "!VS%%fCOMNTOOLS!\..\..\VC\vcvarsall.bat" SET VCVARSALL=!VS%%fCOMNTOOLS!\..\..\VC\vcvarsall.bat
FOR /D %%f IN ("%ProgramFiles(x86)%\Microsoft Visual Studio\????") DO FOR %%g IN (Community Professional Enterprise) DO IF EXIST "%%f\%%g\VC\Auxiliary\Build\vcvarsall.bat" SET VCVARSALL=%%f\%%g\VC\Auxiliary\Build\vcvarsall.bat
IF "%VCVARSALL%"=="" ECHO Cannot find C compiler environment for 'vcvarsall.bat'. & PAUSE & GOTO :EOF
ECHO Setting environment variables for C compiler... %VCVARSALL%
call "%VCVARSALL%" %ARCH%

ECHO.
ECHO ARCH=%ARCH%
ECHO.
ECHO LIB=%LIB%
ECHO.
ECHO INCLUDE=%INCLUDE%
ECHO.
ECHO LIBPATH=%LIBPATH%
ECHO.
ECHO WINDOWSSDKDIR=%WindowsSdkDir%
ECHO.
ECHO WINDOWSSDKVERSION=%WindowsSDKVersion%
ECHO.

:TEST
ECHO Building test program...
cl -c /EHsc /Tc"main.c" /Tc"zipwriter.c"
IF ERRORLEVEL 1 GOTO ERROR
link /out:zipwriter.exe main zipwriter
IF ERRORLEVEL 1 GOTO ERROR
GOTO END

:ERROR
ECHO ERROR: An error occured.
pause
GOTO END

:END
ENDLOCAL
