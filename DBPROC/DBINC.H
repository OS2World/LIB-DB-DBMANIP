#define INCL_BASE
#include <os2.h>
#define READONLY	0
#define READWRITE	1
#define UNLOCK		0
#define LOCK		1

typedef struct _fldinfo
    {
    CHAR name[11];
    CHAR type;
	CHAR length;
    CHAR decimal;
	}FLDDATA;

typedef struct _dbreport
    {
	CHAR  updatey;
	CHAR  updatem;
	CHAR  updated;
	LONG  maxrec;
	SHORT reclngth;
	}DBREPORT;

//function prototypes
SHORT EXPENTRY AddRec	 (SHORT dbhndl, CHAR FAR *recdata, LONG FAR *recnum, SHORT lockstatus) ;
SHORT EXPENTRY PutRec	 (SHORT dbhndl, CHAR FAR *recdata, LONG recnum) ;
SHORT EXPENTRY GetRec	 (SHORT dbhndl, CHAR FAR *recdata, LONG recnum) ;
SHORT EXPENTRY DeleteRec (SHORT dbhndl, LONG recnum) ;
SHORT EXPENTRY RecoverRec(SHORT dbhndl, LONG recnum) ;
SHORT EXPENTRY LockRec	 (SHORT dbhndl, LONG recnum) ;
SHORT EXPENTRY UnLockRec (SHORT dbhndl, LONG recnum) ;
SHORT EXPENTRY DbOpen	 (CHAR FAR *file, SHORT readwrite) ;
SHORT EXPENTRY DbClose	 (SHORT dbhndl) ;
SHORT EXPENTRY DbCreate  (CHAR FAR *file, SHORT fldcount, FLDDATA FAR *info) ;
SHORT EXPENTRY DbInfo	 (SHORT hndl, DBREPORT FAR *info) ;
SHORT EXPENTRY GetField  (SHORT dbhndl, LONG recnum, CHAR FAR *buff, SHORT fieldnum) ;
SHORT EXPENTRY PutField  (SHORT dbhndl, LONG recnum, CHAR FAR *buff, SHORT fieldnum) ;
SHORT EXPENTRY FldInfo	 (SHORT hndl, SHORT fldnum, FLDDATA FAR *fldinfo) ;
