PROJ = DBPROCS
PROJFILE = DBPROCS.MAK
DEBUG = 0

PWBRMAKE  = pwbrmake
NMAKEBSC1  = set
NMAKEBSC2  = nmake
CC	= cl
CFLAGS_G  = /Alfu /W2 /G2 /D_MT /Zp /ML /BATCH
CFLAGS_D  = /Gi$(PROJ).mdt /Zi /Od /FPa
CFLAGS_R  = /Ot /Oi /Ol /Oe /Og /Gs /FPa
MAPFILE_D  = NUL
MAPFILE_R  = NUL
LFLAGS_G  =  /NOI  /BATCH
LFLAGS_D  = /CO /INC /FAR /PACKC /PACKD /PMTYPE:VIO
LFLAGS_R  = /EXE /FAR /PACKC /PACKD /PMTYPE:VIO
LINKER	= link
ILINK  = ilink
LRF  = echo > NUL
IMPLIB	= implib
LLIBS_R  =	/NOD:LLIBCA LLIBCDLL
LLIBS_D  =	/NOD:LLIBCA LLIBCDLL
LLIBS_G  = os2

DEF_FILE  = DBPROCS.DEF
OBJS  = DBMANIP.obj
SBRS  = DBMANIP.sbr

all: $(PROJ).dll

.SUFFIXES:
.SUFFIXES: .sbr .obj .c

DBMANIP.obj : DBMANIP.C dbmanip.h dberr.h

DBMANIP.sbr : DBMANIP.C dbmanip.h dberr.h


$(PROJ).bsc : $(SBRS)
		$(PWBRMAKE) @<<
$(BRFLAGS) $(SBRS)
<<

$(PROJ).dll : $(DEF_FILE) $(OBJS)
!IF $(DEBUG)
		$(LRF) @<<$(PROJ).lrf
$(RT_OBJS: = +^
) $(OBJS: = +^
)
$@
$(MAPFILE_D)
$(LLIBS_G: = +^
) +
$(LLIBS_D: = +^
) +
$(LIBS: = +^
)
$(DEF_FILE) $(LFLAGS_G) $(LFLAGS_D);
<<
!ELSE
		$(LRF) @<<$(PROJ).lrf
$(RT_OBJS: = +^
) $(OBJS: = +^
)
$@
$(MAPFILE_R)
$(LLIBS_G: = +^
) +
$(LLIBS_R: = +^
) +
$(LIBS: = +^
)
$(DEF_FILE) $(LFLAGS_G) $(LFLAGS_R);
<<
!ENDIF
!IF $(DEBUG)
		$(ILINK) -a -e "$(LINKER) @$(PROJ).lrf" $@
!ELSE
		$(LINKER) @$(PROJ).lrf
!ENDIF
		$(IMPLIB) $*.lib $@


.c.sbr :
!IF $(DEBUG)
		$(CC) /Zs $(CFLAGS_G) $(CFLAGS_D) /FR$@ $<
!ELSE
		$(CC) /Zs $(CFLAGS_G) $(CFLAGS_R) /FR$@ $<
!ENDIF

.c.obj :
!IF $(DEBUG)
		$(CC) /c $(CFLAGS_G) $(CFLAGS_D) /Fo$@ $<
!ELSE
		$(CC) /c $(CFLAGS_G) $(CFLAGS_R) /Fo$@ $<
!ENDIF


run: $(PROJ).dll
		

debug: $(PROJ).dll
		
