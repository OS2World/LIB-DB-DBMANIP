#define  MAXRECLNGTH    4000
#define  MAXFIELDS      128
#define  MAXFLDLNGTH    128
#define  MAXNUMLNGTH    19
#define  MAXDECLNGTH    15
#define  MAXDB          30
#define  READONLY       0
#define  READWRITE      1
#define  UNLOCK         0
#define  LOCK           1
#define  RECLNGTH       dbdata[curdb].reclngth
#define  RECSTART       dbdata[curdb].hdrlngth


#ifndef INCL_BASE  /*if not using OS/2*/
    typedef SHORT int
    typedef USHORT unsigned int
    typedef LONG long
    typedef ULONG unsigned long
    typedef VOID void
    typedef EXPENTRY far pascal _loadds
    typedef SEL unsigned int
    typedef HFILE unsigned int
#endif

typedef struct _dbheader
    {
    CHAR  version;
    CHAR  updatey;
    CHAR  updatem;
    CHAR  updated;
    LONG  maxrec;
    SHORT hdrlngth;
    SHORT reclngth;
    CHAR  reserved[20];
    }DBHEADER;

typedef struct _dbreport
    {
    CHAR  updatey;
    CHAR  updatem;
    CHAR  updated;
    LONG  maxrec;
    SHORT reclngth;
    SHORT numflds ;
    }DBREPORT;


typedef struct _fldheader
    {
    CHAR name[11];
    CHAR type;
    CHAR reserved1[4];
    CHAR length;
    CHAR decimal;
    CHAR reserved2[14];
    }FLDHEADER;

typedef struct _fldinfo
    {
    CHAR name[11];
    CHAR type;
    CHAR length;
    CHAR decimal;
    }FLDDATA;


typedef struct _dbinfo
    {
    CHAR  version;
    CHAR  updatey;
    CHAR  updatem;
    CHAR  updated;
    LONG  maxrec;
    SHORT hdrlngth;
    SHORT reclngth;
    SEL   fieldinfo ;
    }DBINFO;


static HFILE    dbptr[MAXDB] ;
static DBHEADER dbdata[MAXDB] ;
static SHORT    openmode[MAXDB] ;
static SHORT    fldcnt[MAXDB] ;
static SHORT    curdb ;
static VOID *   fieldinfo[MAXDB] = {NULL} ;

/*
function prototypes
*/

/*
    Exported Functions
*/
SHORT EXPENTRY AddRec     (SHORT dbhndl, CHAR *recdata, LONG *recnum, SHORT lockstatus) ;
SHORT EXPENTRY PutRec     (SHORT dbhndl, CHAR *recdata, LONG recnum) ;
SHORT EXPENTRY GetRec     (SHORT dbhndl, CHAR *recdata, LONG recnum) ;
SHORT EXPENTRY DeleteRec  (SHORT dbhndl, LONG recnum) ;
SHORT EXPENTRY RecoverRec (SHORT dbhndl, LONG recnum) ;
SHORT EXPENTRY CheckRec   (SHORT dbhndl, LONG recnum) ;
SHORT EXPENTRY LockRec    (SHORT dbhndl, LONG recnum) ;
SHORT EXPENTRY UnLockRec  (SHORT dbhndl, LONG recnum) ;
SHORT EXPENTRY DbCreate   (CHAR *file, SHORT fldcount, FLDDATA *info) ;
SHORT EXPENTRY DbInfo     (SHORT hndl, DBREPORT *info) ;
SHORT EXPENTRY DbOpen     (CHAR *file, SHORT readwrite) ;
SHORT EXPENTRY DbClose    (SHORT dbhndl) ;
SHORT EXPENTRY GetField   (SHORT dbhndl, LONG recnum, CHAR *buff, SHORT fieldnum) ;
SHORT EXPENTRY PutField   (SHORT dbhndl, LONG recnum, CHAR *buff, SHORT fieldnum) ;
SHORT EXPENTRY FldInfo    (SHORT hndl, SHORT fldnum, FLDDATA FAR *fldinfo) ;


/*
    Internal Functions
*/
ULONG RecordSeek (LONG recnum) ;
SHORT RecordWrite(CHAR *record, LONG recnum) ;
SHORT RecordRead (CHAR *record, LONG recnum) ;
SHORT UpdateDBheader(SHORT direction) ;
SHORT RefressDBheader(VOID) ;
SHORT FldName2Num(CHAR *name) ;
SHORT FieldOffset(SHORT fldnum) ;
SHORT FieldLength(SHORT fldnum) ;
CHAR  *rtrim(CHAR *text) ;
CHAR  *ltrim(CHAR *text) ;
SHORT FieldType(SHORT fldnum) ;
SHORT FieldDecimal(SHORT fldnum) ;
SHORT FormatNumber(SHORT fldnum, CHAR *text) ;
VOID  *MemAlloc(USHORT size) ;
VOID  MemFree(VOID *ptr) ;
SHORT Write(USHORT handle, VOID *data, USHORT length) ;
SHORT Read (USHORT handle, VOID *data, USHORT length) ;
SHORT Seek(LONG distance) ;
SHORT FileSize(VOID) ;
SHORT Lock(LONG offset, LONG range, BOOL action) ;
SHORT GetDate(CHAR *day, CHAR *month, CHAR *year) ;
VOID  Sleep(LONG time) ;
SHORT Close (USHORT) ;
SHORT Open(CHAR *name, ULONG *handle, USHORT attribute, USHORT mode) ;
