/* vim: set tabstop=3 expandtab:
**
** This file is in the public domain.
**
** osd.c
**
** $Id: osd.c,v 1.2 2001/04/27 14:37:11 neil Exp $
**
*/

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <shlwapi.h>
#else
#include <sys/time.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include <noftypes.h>
#include <nofconfig.h>
#include <log.h>
#include <osd.h>
#include <nofrendo.h>

#include <version.h>

char configfilename[]="na";

/* This is os-specific part of main() */
int osd_main(const char* filename)
{
   config.filename = configfilename;

   return main_loop(filename, system_autodetect);
}

/* File system interface */
void osd_fullname(char *fullname, const char *shortname)
{
   strncpy(fullname, shortname, PATH_MAX);
}

/* This gives filenames for storage of saves */
char *osd_newextension(char *string, const char *ext)
{
    int l = strlen(string);
    while (l && string[l] != '.') {
        l--;
    }
    if (l) string[l] = 0;
    strcat(string, ext);
#ifdef _WIN32
    char buffer[PATH_MAX];
    GetCurrentDirectoryA(PATH_MAX, buffer);
    PathCombineA(string, buffer, string);
#endif
    return string;
}

/* This gives filenames for storage of PCX snapshots */
int osd_makesnapname(char *filename, int len)
{
   return -1;
}

#ifdef _WIN32

OSDFile* OSDFile::fopen(char const* _FileName, char const* _Mode)
{
    Win32File* pFile = new Win32File;
    pFile->fp = ::fopen(_FileName, _Mode);
    return pFile;
}


int Win32File::fclose()
{
    int result = ::fclose(fp);
    delete this;
    return result;
}

size_t  Win32File::fread(void* _Buffer, size_t _ElementSize, size_t _ElementCount)
{
    size_t result = ::fread(_Buffer, _ElementSize, _ElementCount, fp);
    return result;
}

size_t Win32File::fwrite(void const* _Buffer, size_t _ElementSize, size_t _ElementCount)
{
    return ::fwrite(_Buffer, _ElementSize, _ElementCount, fp);
}

long Win32File::ftell()
{
    return ::ftell(fp);
}

int Win32File::fseek(long _Offset, int _Origin)
{
    return ::fseek(fp, _Offset, _Origin);
}

#endif // #ifdef _WIN32