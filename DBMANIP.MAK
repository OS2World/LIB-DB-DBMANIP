# IBM Developer's Workframe/2 Make File Creation run at 22:46:35 on 07/14/92

# Make File Creation run in directory:
#   F:\DBPUBLIC;

.SUFFIXES:

.SUFFIXES: .c

DBMANIP.DLL:  \
  DBMANIP.OBJ \
  DBMANIP.DEF \
  DBMANIP.MAK
   @REM @<<DBMANIP.@0
     /CO /NOL /PM:VIO +
     DBMANIP.OBJ
     DBMANIP.DLL
     
     
     DBMANIP.DEF;
<<
   LINK386.EXE @DBMANIP.@0
  IMPLIB DBMANIP.LIB DBMANIP.DEF

{.}.c.obj:
   ICC.EXE /C /Ls /Li /Lx /La /Ti /Ge- .\$*.c

!include DBMANIP.DEP
