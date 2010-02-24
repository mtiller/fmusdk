@echo off 
rem ------------------------------------------------------------
rem This batch builds an FMU of the FMU SDK
rem Usage: build_fmu <fmu_dir_name>
rem (c) 2010 QTronic GmbH
rem ------------------------------------------------------------

echo building FMU %1

rem setup the compiler
if defined VS90COMNTOOLS (call "%VS90COMNTOOLS%\vsvars32.bat") else ^
if defined VS80COMNTOOLS (call "%VS80COMNTOOLS%\vsvars32.bat") else ^
goto noCompiler

rem create the %1.dll in the temp dir
if not exist temp mkdir temp 
pushd temp
del *.dll
rem /wd4090 to disable warnings about different 'const' qualifiers
rem cl /LDd /Fd%1.pdb ..\%1\%1.c /I ..\include
cl /LD /wd4090 ..\%1\%1.c /I ..\include
if not exist %1.dll goto compileError

rem create FMU dir structure with root 'fmu'
set BIN_DIR=fmu\binaries\win32
set SRC_DIR=fmu\sources
set DOC_DIR=fmu\documentation
if not exist %BIN_DIR% mkdir %BIN_DIR%
if not exist %SRC_DIR% mkdir %SRC_DIR%
if not exist %DOC_DIR% mkdir %DOC_DIR%
move /Y %1.dll %BIN_DIR%
del /Q ..\%1\*~
copy ..\%1\%1.c %SRC_DIR% 
copy ..\%1\modelDescription.xml fmu
copy ..\%1\model.png fmu
if not %1==dahlquist copy ..\include\fmuTemplate.c %SRC_DIR%
if not %1==dahlquist copy ..\include\fmuTemplate.h %SRC_DIR%
copy ..\%1\*.html %DOC_DIR%
copy ..\%1\*.png  %DOC_DIR%
del %DOC_DIR%\model.png 

rem zip the directory tree 
cd fmu
set ZIP_FILE=..\..\..\fmu\%1.fmu
if exist %ZIP_FILE% del %ZIP_FILE%
..\..\..\bin\7z.exe a -tzip -xr!.svn %ZIP_FILE% ^
  modelDescription.xml model.png binaries sources documentation
goto cleanup

:noCompiler
echo No Microsoft Visual C compiler found
exit

:compileError
echo build of %1 failed

:cleanup
popd
if exist temp rmdir /S /Q temp
echo done.



