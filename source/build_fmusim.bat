@echo off 
rem ------------------------------------------------------------
rem This batch builds fmusim.exe
rem (c) 2010 QTronic GmbH
rem ------------------------------------------------------------

echo building fmusim.exe

rem setup the compiler
if defined VS90COMNTOOLS (call "%VS90COMNTOOLS%\vsvars32.bat") else ^
if defined VS80COMNTOOLS (call "%VS80COMNTOOLS%\vsvars32.bat") else ^
goto noCompiler

set SRC=main.c xml_parser.c stack.c fmuinit.c fmusim.c fmuio.c fmuzip.c

rem create fmusim.exe in the fmusim dir
pushd fmusim
rem /wd4090 to disable warnings about different 'const' qualifiers
rem cl %SRC% /Fefmusim.exe /Fdfmusim.pdb /MTd /I..\include /link libexpatMT.lib /NODEFAULTLIB:LIBCMT 
cl %SRC% /wd4090 /Fefmusim.exe /I..\include /link libexpatMT.lib  
del *.obj
popd
if not exist fmusim\fmusim.exe goto compileError
move /Y fmusim\fmusim.exe ..\bin
goto done

:noCompiler
echo No Microsoft Visual C compiler found

:compileError
echo build of fmusim.exe failed

:done
echo done.



