@echo off 

rem ------------------------------------------------------------
rem This batch builds the simulator and demo FMUs of the FmuSDK
rem (c) 2010 QTronic GmbH
rem ------------------------------------------------------------

echo Making the fmu simulator ...
call build_fmusim

echo Making the FMUs of the FmuSDK ...
call build_fmu dq
call build_fmu inc
call build_fmu values
call build_fmu bouncingBall


