#define INCL_BASE
#define INCL_DOSFILEMGR
#define INCL_DOSDATETIME
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dbmanip.h"
#include "dberr.h"


SHORT EXPENTRY DbCreate(CHAR *file, SHORT fldcount, FLDDATA *info)
{
DBHEADER header ;
FLDHEADER fldhdr ;
SHORT x, rc ;
CHAR filename[256], EOHeader[2] = {0xD, 0x1A}, day, month, year ;
HFILE hndl ;
USHORT action, fsOpenMode = OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYREADWRITE ;

/*check passed information*/
if(file[0] == 0 || file == NULL)
    return DB_FILE_NAME_ERR ;

strcpy(filename, file) ;
if(strstr(strupr(filename), ".DBF") == NULL)
    strcat(filename, ".DBF") ;

if(fldcount > MAXFIELDS)
    return DB_MAX_FIELDS ;

for(x = 0 ; x < fldcount ; x++)
    {
    if(strlen(info[x].name) > 10 || strlen(info[x].name) < 1)
        return DB_BAD_FLDNAME ;
    switch(info[x].type)
        {
        case 'C':
        case 'c':
            if(info[x].length > MAXFLDLNGTH || info[x].length < 1)
                return DB_FLD_LNGTH ;
            info[x].decimal = 0 ;
            break ;
        case 'D':
        case 'd':
            info[x].length = 8 ;
            info[x].decimal = 0 ;
            break ;
        case 'L':
        case 'l':
            info[x].length = 1 ;
            info[x].decimal = 0 ;
            break ;
        case 'N':
        case 'n':
            if(info[x].length > MAXNUMLNGTH || info[x].length < 1)
                return DB_FLD_LNGTH ;
            if(info[x].decimal < 0 || info[x].decimal > MAXDECLNGTH || info[x].decimal > info[x].length - 2)
                return DB_DEC_LNGTH ;
            break ;
        case 'M':
        case 'm':
            info[x].length = 8 ;
            info[x].decimal = 0 ;
            break ;
        default:
            return DB_INVALID_FIELD ;
        }
    }

GetDate(&day, &month, &year) ;
memset(header.reserved, 0, 20) ;

header.reclngth = 1 ;    /*1 larger than requested to hold deleted marker*/
for(x = 0 ; x < fldcount ; x++)
    header.reclngth += info[x].length ;

if(header.reclngth > MAXRECLNGTH)
    return DB_REC_TOO_LONG ;
header.hdrlngth = (sizeof(DBHEADER) + 1 + (sizeof(FLDHEADER) * fldcount) );
header.version    = 3 ;
header.updatey    = year;
header.updatem    = month;
header.updated    = day;
header.maxrec    = 0L ;
action = Open(filename, &hndl, FILE_TRUNCATE | FILE_CREATE , fsOpenMode) ;
if(action)
    return action ;
rc = OK ;
if(Write(hndl, &header, sizeof(header) ) )
    rc = DB_WRITE_ERR ;
else
    {
    memset(fldhdr.reserved1, 0, 4) ;
    memset(fldhdr.reserved2, 0, 14) ;
    for(x = 0 ; x < fldcount ; x++)
        {
        memset(fldhdr.name, 0, 11) ;
        fldhdr.type = toupper(info[x].type) ;
        strcpy(fldhdr.name, info[x].name ) ;
        strupr(fldhdr.name) ;
        fldhdr.length = info[x].length ;
        fldhdr.decimal = info[x].decimal ;
        if(Write(hndl, &fldhdr, sizeof(fldhdr)) )
            {
            rc = DB_WRITE_ERR ;
            break ;
            }
        }
    }
if (Write(hndl, EOHeader, 2) )
    rc = DB_WRITE_ERR ;
Close(hndl) ;
if(rc)
    remove(filename) ;
return rc ;
}


SHORT EXPENTRY DbOpen(CHAR *file, SHORT readwrite)
{
CHAR filename[256] ;
USHORT action, fsOpenMode ;
FLDHEADER *flds = NULL;
SHORT rc = OK, x ;

strcpy(filename, file) ;
if(strstr(strupr(filename), ".DBF") == NULL)
    strcat(filename, ".DBF") ;

/*find free file handle entry*/
for (curdb = 0; curdb < MAXDB; curdb++)
    if (dbptr[curdb] == 0)
        break;

/*all file handle entries in use*/
if (curdb == MAXDB)
    return DB_NO_HANDLES;

openmode[curdb] = readwrite ;

if(readwrite)
    /* get access to write to the database*/
    fsOpenMode = OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE |
                  OPEN_FLAGS_NO_CACHE | OPEN_FLAGS_WRITE_THROUGH;
else
    fsOpenMode = OPEN_ACCESS_READONLY | OPEN_SHARE_DENYNONE ;

if( (action = Open(filename, &dbptr[curdb], FILE_OPEN, fsOpenMode)) != OK)
    return action ;

if(RefressDBheader())
    {
    Close(dbptr[curdb]) ;
    dbptr[curdb] = 0 ;
    return DB_UPDATE_ERR ;
    }

fldcnt[curdb] = (RECSTART - sizeof(DBHEADER)) / sizeof(FLDHEADER) ;

if((flds = (FLDHEADER *) MemAlloc(fldcnt[curdb] * sizeof(FLDHEADER))) == NULL)
    {
    Close(dbptr[curdb]) ;
    dbptr[curdb] = 0 ;
    return DB_MEM_ERR ;
    }

fieldinfo[curdb] = flds ;
if(Seek(sizeof(DBHEADER)))
    rc = DB_SEEK_ERR ;
else
    {
    for(x = 0 ; x < fldcnt[curdb]; x++)
        {
        if(Read(dbptr[curdb], &flds[x], sizeof(FLDHEADER)) )
            {
            rc = DB_READ_ERR ;
            break ;
            }
        }
    }
if(rc)
    {
    MemFree(flds) ;
    Close(dbptr[curdb]) ;
    dbptr[curdb] = 0 ;
    curdb = rc ;
    }
return curdb ;
}


SHORT EXPENTRY DbClose(SHORT dbhndl)
{
curdb = dbhndl ;

if(curdb < 0 || curdb >= MAXDB || dbptr[curdb] == 0)
    return DB_INVALID_HANDLE ;

MemFree(fieldinfo[curdb]) ;
Close(dbptr[curdb]) ;
dbptr[curdb] = 0 ;
return 0 ;
}

SHORT EXPENTRY DbInfo(SHORT hndl, DBREPORT *info)
{
SHORT rc ;

curdb = hndl ;

if(curdb < 0 || curdb >= MAXDB || dbptr[curdb] == 0)
    rc = DB_INVALID_HANDLE ;

else if(RefressDBheader() )
    rc = DB_READ_ERR ;
else
    {
    info->updatey =  dbdata[curdb].updatey ;
    info->updatem =  dbdata[curdb].updatem ;
    info->updated =  dbdata[curdb].updated ;
    info->maxrec  =  dbdata[curdb].maxrec ;
    info->reclngth = RECLNGTH ;
    info->numflds =(SHORT) ((RECSTART - sizeof(DBHEADER) - 1) / sizeof(FLDHEADER)) ;
    rc = OK ;
    }
return rc ;
}

SHORT EXPENTRY FldInfo(SHORT hndl, SHORT fldnum, FLDDATA FAR *fldinfo)
{
FLDHEADER *fldheader ;

curdb = hndl ;
if(curdb < 0 || curdb >= MAXDB || dbptr[curdb] == 0)
    return DB_INVALID_HANDLE ;

if(fldnum >= fldcnt[curdb])
    return DB_INVALID_FIELD ;

fldheader = fieldinfo[curdb] ;
memcpy(fldinfo->name, fldheader[fldnum].name, 11) ;
fldinfo->type = fldheader[fldnum].type ;
fldinfo->length = fldheader[fldnum].length ;
fldinfo->decimal = fldheader[fldnum].decimal ;
return 0 ;
}



SHORT EXPENTRY AddRec(SHORT dbhndl, CHAR *recdata, LONG *recnum, SHORT lockstatus)
{
SHORT rc = OK, rc2 = OK ;
ULONG end ;
CHAR *newrec = NULL;
LONG Offset, Range ;

curdb = dbhndl ;

if(curdb < 0 || curdb >= MAXDB || dbptr[curdb] == 0)
    return DB_INVALID_HANDLE ;

if(openmode[curdb] == READONLY)
    return DB_READONLY ;

/*
  Allocate a buffer for the New record.  if NULL is sent as
  the record pointer then set to all spaces.  Add this to the
  database as the new record.
*/
newrec = (CHAR *) MemAlloc(RECLNGTH) ;
if(newrec == NULL)
    return DB_MEM_ERR ;

if(recdata == NULL)
    memset(newrec, ' ', RECLNGTH) ;
else
    memcpy(newrec, recdata, RECLNGTH) ;

/*
 Lock the database while we add the new record to the database.
 Compute the new records number and set the database header to
 reflect the new maxrecords value.

 Then unlock the total database and if requested by calling routine
 lock the new record.
*/
end = FileSize() ;
if(end  < RECSTART)
   return DB_SEEK_ERR ;

Offset = 0L ;
Range =  (LONG) end ;

if(Lock(Offset, Range, LOCK))
    rc =  DB_LOCK_ERR ;
else if (RefressDBheader())
    rc = DB_UPDATE_ERR ;
else if (RecordWrite(newrec, dbdata[curdb].maxrec) )
    rc = DB_WRITE_ERR ;
else
    {
    Lock(Offset, Range, UNLOCK) ;
    *recnum = dbdata[curdb].maxrec ;
    rc = UpdateDBheader(1) ;
    }


if(lockstatus  && rc == OK)
    {
    Offset = (LONG) end ;
    Range = (LONG) RECLNGTH ;
    if(Lock(Offset, Range, LOCK) )
        rc = DB_LOCK_ERR ;
    }
if(newrec != NULL)
    MemFree(newrec) ;
return rc;
}


SHORT EXPENTRY PutRec(SHORT dbhndl, CHAR *recdata, LONG recnum)
{
SHORT rc = OK;

curdb = dbhndl ;

if(curdb < 0 || curdb >= MAXDB || dbptr[curdb] == 0)
    rc = DB_INVALID_HANDLE ;
else if(openmode[curdb] == READONLY)
     rc = DB_READONLY ;
else if(RefressDBheader())
    rc = DB_READ_ERR ;
else if(recnum < 0 || recnum >= dbdata[curdb].maxrec)
    rc = DB_INVALID_RECORD ;

else if (RecordWrite(recdata, recnum))
    rc = DB_WRITE_ERR ;
return rc;
}


SHORT EXPENTRY GetRec(SHORT dbhndl, CHAR *recdata, LONG recnum)
{
SHORT rc = OK;

curdb = dbhndl ;

if(curdb < 0 || curdb >= MAXDB || dbptr[curdb] == 0)
    rc = DB_INVALID_HANDLE ;
else if(RefressDBheader())
    rc = DB_READ_ERR ;
else if(recnum < 0 || recnum >= dbdata[curdb].maxrec)
    rc = DB_INVALID_RECORD ;

else if (RecordRead(recdata, recnum))
        rc = DB_READ_ERR ;

return rc;
}

SHORT EXPENTRY DeleteRec(SHORT dbhndl, LONG recnum)
{
SHORT rc = OK;

curdb = dbhndl ;

if(curdb < 0 || curdb >= MAXDB || dbptr[curdb] == 0)
    rc = DB_INVALID_HANDLE ;
else if(RefressDBheader())
    rc = DB_READ_ERR ;
else if(recnum < 0 || recnum >= dbdata[curdb].maxrec)
    rc = DB_INVALID_RECORD ;

else if(openmode[curdb] == READONLY)
    rc = DB_READONLY ;

else if (RecordSeek(recnum) == 0)
    rc = DB_SEEK_ERR ;

else if (Write(dbptr[curdb], "*", 1))
    rc = DB_WRITE_ERR ;

return rc ;
}


SHORT EXPENTRY RecoverRec(SHORT dbhndl, LONG recnum)
{
CHAR record;
SHORT rc = OK;

curdb = dbhndl ;

if(curdb < 0 || curdb >= MAXDB || dbptr[curdb] == 0)
    rc = DB_INVALID_HANDLE ;
else if(openmode[curdb] == READONLY)
    rc = DB_READONLY ;
else if(RefressDBheader())
    rc = DB_READ_ERR ;
else if(recnum < 0 || recnum >= dbdata[curdb].maxrec)
    rc = DB_INVALID_RECORD ;
else if (RecordSeek(recnum) == 0)
    rc = DB_SEEK_ERR ;
else if (Read(dbptr[curdb], &record, 1))
    rc = DB_READ_ERR ;
else if(record == ' ')
    rc = DB_NOT_DELETED ;
else if (RecordSeek(recnum) == 0)
    rc = DB_SEEK_ERR ;
else if (Write(dbptr[curdb], " ", 1))
    rc = DB_WRITE_ERR ;

return rc ;
}

SHORT EXPENTRY CheckRec(SHORT dbhndl, LONG recnum)
{
CHAR record;
SHORT rc;

curdb = dbhndl ;

if(curdb < 0 || curdb >= MAXDB || dbptr[curdb] == 0)
    rc = DB_INVALID_HANDLE ;
else if(RefressDBheader())
    rc = DB_READ_ERR ;
else if(recnum < 0 || recnum >= dbdata[curdb].maxrec)
    rc = DB_INVALID_RECORD ;
else
    {
    if(RecordSeek(recnum) == 0)
        rc = DB_SEEK_ERR ;
    else
        {
        if(Read(dbptr[curdb], &record, sizeof(record)) )
            rc = DB_READ_ERR ;
        else
            {
            if(record == '*')
                rc = 0 ;
            else
                rc = 1 ;
            }
        }
    }
return rc ;
}

SHORT EXPENTRY LockRec(SHORT dbhndl, LONG recnum)
{
LONG Offset, Range ;
SHORT x, rc = OK;

curdb = dbhndl ;

if(curdb < 0 || curdb >= MAXDB || dbptr[curdb] == 0)
    rc = DB_INVALID_HANDLE ;

else if(openmode[curdb] == READONLY)
    rc = DB_READONLY ;
else if(RefressDBheader())
    rc = DB_READ_ERR ;
else if(recnum < 0 || recnum >= dbdata[curdb].maxrec)
    rc = DB_INVALID_RECORD ;
else
    {
    if( (Offset = RecordSeek(recnum) ) == 0)
        rc = DB_SEEK_ERR ;
    else
        {
        Range = RECLNGTH ;
        rc = Lock(Offset, Range, LOCK) ;
        }
    }
return rc ;
}


SHORT EXPENTRY UnLockRec(SHORT dbhndl, LONG recnum)
{
LONG Offset, Range ;
SHORT x, rc;

curdb = dbhndl ;

if(curdb < 0 || curdb >= MAXDB || dbptr[curdb] == 0)
    rc = DB_INVALID_HANDLE ;

else if(openmode[curdb] == READONLY)
    rc = DB_READONLY ;
else if(RefressDBheader())
    rc = DB_READ_ERR ;
else if(recnum < 0 || recnum >= dbdata[curdb].maxrec)
    rc = DB_INVALID_RECORD ;

else
    {
    Offset = RecordSeek(recnum) ;
    if(Offset == 0)
        rc = DB_SEEK_ERR ;
    else
        {
        Range = RECLNGTH ;
        rc = Lock(Offset, Range, UNLOCK) ;
        }
    }
return rc ;
}


SHORT EXPENTRY GetField(SHORT dbhndl, LONG recnum, CHAR *buff, SHORT fldnm)
{
static CHAR recbuff[MAXRECLNGTH + 1], fldbuff[MAXFLDLNGTH + 1] ;
static SHORT rc = OK, type, offset, length;

curdb = dbhndl ;
rc = OK ;
memset(recbuff, 0, MAXRECLNGTH + 1) ;
memset(fldbuff, 0, MAXFLDLNGTH + 1) ;

if(curdb < 0 || curdb >= MAXDB || dbptr[curdb] == 0)
    rc = DB_INVALID_HANDLE ;

else if(RefressDBheader())
    rc = DB_READ_ERR ;
else if (recnum < 0 || recnum >= dbdata[curdb].maxrec)
        rc = DB_INVALID_RECORD ;
else if (RecordRead(recbuff, recnum) )
        rc = DB_READ_ERR ;
else
    {
    type = FieldType(fldnm) ;
    offset = FieldOffset(fldnm) ;
    length = FieldLength(fldnm) ;
    switch(type)
        {
        case 'C':
            memcpy(fldbuff, &recbuff[offset], length) ;
            strcpy(buff, rtrim(fldbuff)) ;
            break ;
        case 'L':
            buff[0] = recbuff[offset] ;
            buff[1] = 0 ;
            break ;
        case 'D':
            memmove(fldbuff, &recbuff[offset + 4], 2) ;
            fldbuff[2] = '/' ;
            memmove(&fldbuff[3], &recbuff[offset + 6], 2) ;
            fldbuff[5] = '/' ;
            memmove(&fldbuff[6], &recbuff[offset + 2], 2) ;
            strcpy(buff, fldbuff) ;
            break ;
        case 'N':
            memcpy(fldbuff, &recbuff[offset], length) ;
            strcpy(buff, ltrim(fldbuff) ) ;
            break ;
        default:
            rc = DB_INVALID_FIELD ;
            break ;
        }
    }
return rc ;
}



SHORT EXPENTRY PutField(SHORT dbhndl, LONG recnum, CHAR *buff, SHORT fldnm)
{
static CHAR recbuff[MAXRECLNGTH + 1], fldbuff[MAXFLDLNGTH + 1] ;
static SHORT x, rc, type, offset, length ;

rc = OK ;
curdb = dbhndl ;
memset(recbuff, 0, MAXRECLNGTH + 1) ;
memset(fldbuff, 0, MAXFLDLNGTH + 1) ;
strcpy(fldbuff, buff) ;

if(curdb < 0 || curdb >= MAXDB || dbptr[curdb] == 0)
    rc = DB_INVALID_HANDLE ;
else if(openmode[curdb] == READONLY)
    rc = DB_READONLY ;
else if(RefressDBheader())
    rc = DB_READ_ERR ;
else if (recnum < 0 || recnum >= dbdata[curdb].maxrec)
        rc = DB_INVALID_RECORD ;
else if (RecordRead(recbuff, recnum) )
        rc = DB_READ_ERR ;
else
    {
    type = FieldType(fldnm) ;
     offset = FieldOffset(fldnm) ;
    length = FieldLength(fldnm) ;
    switch(type)
        {
        case 'C':
            if(strlen(fldbuff) > length)
                rc = DB_FLD_LNGTH ;
            else
                {
                for(x = strlen(fldbuff) ; x < length ; x++)
                    fldbuff[x] = ' ' ;
                memcpy(&recbuff[offset], fldbuff, length) ;
                }
            break ;
        case 'L':
            recbuff[offset] = toupper(fldbuff[0]);
            break ;
        case 'D':
            if(strlen(fldbuff) == 8)
                {
                memmove(&recbuff[offset], "19", 2) ;
                memmove(&recbuff[offset + 2], &fldbuff[6], 2) ;
                }
            else if (strlen(fldbuff) == 10)
                memmove(&recbuff[offset], &fldbuff[6], 4) ;
            else
                rc = DB_INVALID_DATE ;
            if(rc == OK)
                {
                memmove(&recbuff[offset + 4], fldbuff, 2) ;
                memmove(&recbuff[offset + 6], &fldbuff[3], 2) ;
                }
            break ;
        case 'N':
            if(FormatNumber(fldnm, fldbuff) )
                rc = DB_INVALID_NUMBER ;
            else
                memcpy(&recbuff[offset], fldbuff, length) ;
            break ;
        default:
            rc = DB_INVALID_FIELD ;
            break ;
        }
    }
if(rc == OK)
    rc = RecordWrite(recbuff, recnum) ;
return rc ;
}


/*
    INTERNAL FUNCTIONS
*/

SHORT UpdateDBheader(SHORT direction)
{
SHORT rc = OK, x, locked = FALSE ;
LONG Offset, Range ;
CHAR day, month, year ;

Offset = 0L ;
Range = sizeof(DBHEADER) ;

GetDate(&day, &month, &year) ;
dbdata[curdb].updatey = year ;
dbdata[curdb].updatem = month;
dbdata[curdb].updated = day;

if(Lock(Offset, Range, LOCK))
    rc = DB_UPDATE_ERR ;
else if (Seek(0L))
    rc = DB_UPDATE_ERR ;
else if(Read(dbptr[curdb], &dbdata[curdb], sizeof(DBHEADER) ) )
    rc = DB_UPDATE_ERR ;
else
    {
    dbdata[curdb].maxrec += direction ;
    if (Seek(0L))
        rc = DB_UPDATE_ERR ;
    else if(Write(dbptr[curdb], &dbdata[curdb], sizeof(DBHEADER)) )
        rc = DB_UPDATE_ERR ;
    }

Lock(Offset, Range, UNLOCK) ;
return rc ;
}


SHORT RefressDBheader(VOID)
{
SHORT rc = OK ;

if (Seek(0L))
    rc = DB_UPDATE_ERR ;
else if(Read(dbptr[curdb], &dbdata[curdb], sizeof(DBHEADER) ) )
    rc = DB_UPDATE_ERR ;
return rc ;
}



SHORT RecordWrite(CHAR *record, LONG recnum)
{
SHORT rc = OK ;

if(RecordSeek(recnum) == 0 )
    rc = DB_SEEK_ERR ;
else if (Write(dbptr[curdb], record, RECLNGTH))
    rc = DB_WRITE_ERR ;
return rc ;
}

SHORT RecordRead(CHAR *record, LONG recnum)
{
SHORT rc = OK ;

if(RecordSeek(recnum) == 0)
    rc = DB_SEEK_ERR ;
else if (Read(dbptr[curdb], record, RECLNGTH))
    rc = DB_READ_ERR ;
return rc ;
}

SHORT FieldDecimal(SHORT fldnum)
{
FLDHEADER *fldinfo ;

if(fldnum >= fldcnt[curdb])
    return DB_INVALID_FIELD ;
fldinfo = (FLDHEADER *) fieldinfo[curdb] ;
return(fldinfo[fldnum].decimal) ;
}


SHORT FieldType(SHORT fldnum)
{
FLDHEADER *fldinfo ;

if(fldnum >= fldcnt[curdb])
    return DB_INVALID_FIELD ;

fldinfo = (FLDHEADER *) fieldinfo[curdb] ;
return(fldinfo[fldnum].type) ;
}

SHORT FieldOffset(SHORT fldnum)
{
SHORT offset, x;
FLDHEADER *fldinfo ;

if(fldnum >= fldcnt[curdb])
    return DB_INVALID_FIELD ;

offset = 1 ;
fldinfo = (FLDHEADER *) fieldinfo[curdb] ;
for(x = 0 ;  x < fldnum ; x++)
    offset += fldinfo[x].length ;
return offset ;
}

SHORT FieldLength(SHORT fldnum)
{
FLDHEADER *fldinfo ;

if(fldnum >= fldcnt[curdb])
    return DB_INVALID_FIELD ;

fldinfo = (FLDHEADER *) fieldinfo[curdb] ;
return(fldinfo[fldnum].length) ;
}

SHORT FldName2Num(CHAR *name)
{
SHORT x ;
FLDHEADER *fldinfo ;
CHAR fldname[12] ;
strcpy(fldname, strupr(name)) ;

fldinfo = (FLDHEADER *) fieldinfo[curdb] ;

for(x = 0 ; x < fldcnt[curdb] ; x++)
    {
    if(strcmp(fldname, strupr(fldinfo[x].name) ) == 0)
        break ;
    }
if(x >= fldcnt[curdb] )
    x = DB_INVALID_FIELD ;
return x ;
}

CHAR *rtrim(CHAR *text)
{
SHORT x ;
x = strlen(text) - 1 ;
while(text[x] == ' ')
    text[x--] = 0 ;
return text ;
}

CHAR *ltrim(CHAR *text)
{
SHORT x ;
x = 0 ;
while(text[x] == ' ') x++ ;
return &text[x] ;
}

SHORT FormatNumber(SHORT fldnum, CHAR *text)
{
SHORT origlength,
    origright,
    origleft,
    destright,
    destleft,
    destlength,
    pad,
    x ;
CHAR frmttext[20], *period ;

destlength = FieldLength(fldnum) ;
destright = FieldDecimal(fldnum) ;
if(destright > 0)
    destright ++ ; /*add for decimal*/
else
    destright = 0 ;
destleft = destlength - destright ;
origlength = strlen(text) ;
if( (period = strchr(text, '.')) != NULL)
    origright = origlength - strcspn(text, ".") ;
else
    origright = 0 ;
origleft = origlength - origright ;
if(origleft > destleft)
    return DB_INVALID_NUM ;
pad = destleft - origleft;

/*left pad with spaces and move left side of decimal over*/
for(x = 0 ;  x < pad; x++)
    frmttext[x] = ' ' ;
memcpy(&frmttext[x], text, origleft) ;

if(destright > 0)
    {
    frmttext[destleft] = '.' ;
    if(origright > 0)
        memcpy(&frmttext[destleft + 1], period + 1, min(destright, origright)) ;
    if(destright > origright)
        {
        for(x = destleft + origright + 1; x < destlength ; x++)
            frmttext[x] ='0' ;
        }
    }
strcpy(text, frmttext) ;
return OK ;
}


/*
    All functions with OS/2 Calls are located below
*/

ULONG RecordSeek(LONG recnum)
{
ULONG newpos ;
if(DosSetFilePtr(dbptr[curdb],
                (LONG) RECSTART + ((LONG) recnum * (LONG) RECLNGTH),
                 FILE_BEGIN, &newpos) )
    newpos = 0 ;
return newpos ;
}

VOID *MemAlloc(USHORT size)
{
VOID *ptr ;
ptr = malloc((size_t)size) ;
return ptr ;
}

VOID MemFree(VOID *ptr)
{
free(ptr) ;
}

SHORT Write(USHORT handle, VOID *data, USHORT length)
{
ULONG written ;
SHORT rc = OK;

if(DosWrite(handle, data, length, &written))
    rc = DB_WRITE_ERR ;
else if(written != length)
    rc = DB_WRITE_ERR ;
return rc ;
}

SHORT Read(USHORT handle, VOID *data, USHORT length)
{
ULONG read ;
SHORT rc = OK;
APIRET apiret = 0 ;
if((apiret = DosRead(handle, data, length, &read)) != 0)
    rc = DB_READ_ERR ;
else if(read != length)
    rc = DB_READ_ERR ;
return rc ;
}

SHORT Seek(LONG distance)
{
ULONG newpos ;
SHORT rc = OK ;
APIRET apiret = 0 ;
if((apiret = DosSetFilePtr(dbptr[curdb], distance, FILE_BEGIN, &newpos) ) != 0)
    rc = DB_SEEK_ERR ;
return rc ;
}

SHORT FileSize(VOID)
{
ULONG size ;
APIRET rc ;
if((rc = DosSetFilePtr(dbptr[curdb], 0L, FILE_END, &size)) != 0)
    size = 0 ;
return size ;
}

SHORT Lock(LONG offset, LONG range, BOOL action)
{
FILELOCK flock, flock2 ;
SHORT rc = OK ;

flock.lOffset = offset ;
flock.lRange = range ;
flock2.lOffset = 0 ;
flock2.lRange = 0 ;
if(action)
    {
    if(DosSetFileLocks(dbptr[curdb], &flock2, &flock, 2000L, 0L ) != 0)
        rc = DB_LOCK_ERR ;
    }
else
    {
    if(DosSetFileLocks(dbptr[curdb], &flock, &flock2, 2000L, 0L ) != 0)
        rc = DB_UNLOCK_ERR ;
    }

return rc ;
}

SHORT GetDate(CHAR *day, CHAR *month, CHAR *year)
{
DATETIME dt ;
DosGetDateTime(&dt) ;
*day = dt.day ;
*month = dt.month ;
*year = dt.year - 1900 ;
}

VOID Sleep(LONG time)
{
DosSleep((ULONG) time) ;
return ;
}

SHORT Close(USHORT handle)
{
if(DosClose((HFILE) handle) )
    return DB_CLOSE_ERR ;
}

SHORT Open(CHAR *name, ULONG *handle, USHORT flags, USHORT mode)
{
ULONG action, rc, x ;

/*make 3 attempts to open file*/
for(x = 0 ; x < 3 ; x++)
    {
    rc = DosOpen(name, handle, &action, 0L,
                     FILE_NORMAL,
                     flags, mode, 0L) ;
    if(rc == 0)
        break ;
    Sleep(100L) ;
    }

if(rc == ERROR_ACCESS_DENIED ||
   rc == ERROR_INVALID_ACCESS)
    return DB_ACCESS_DENIED ;
else if (rc != 0)
    return DB_OPEN_ERR ;
}
