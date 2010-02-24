@echo off 
rem ------------------------------------------------------------
rem To run a simulation, start this batch in this directory. 
rem Example: fmusim fmu/dq.fmu 0.3 0.1 1
rem To build bin\fmusim.exe 
rem and the demo FMUs, run source\build_all.bat
rem ------------------------------------------------------------

set FMUSDK_HOME=.
bin\fmusim.exe %1 %2 %3 %4
