REM ****************************************
REM   Delete working folders
REM ****************************************

IF "%OS%" == "Windows_NT" GOTO WinNT
FOR %%i IN (Debug, Release) DO DELTREE %%i
GOTO CONT2
:WinNT
FOR %%i IN (Obj, Lst) DO IF EXIST %%i RD %%i /S/Q
:CONT2

REM ****************************************
REM   Delete files
REM ****************************************

FOR %%i IN (OBJ, TMP, BAK, HTM, PLG, DEP) DO IF EXIST *.%%i DEL *.%%i
