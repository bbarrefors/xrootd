/******************************************************************************/
/*                                                                            */
/*                        X r d S y s U t i l s . c c                         */
/*                                                                            */
/* (c) 2005 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*                            All Rights Reserved                             */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC02-76-SFO0515 with the Department of Energy              */
/******************************************************************************/

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>

#ifdef __macos__
#include <mach-o/dyld.h>
#endif

#ifdef WIN32
#include <direct.h>
#include "XrdSys/XrdWin32.hh"
#else
#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif
#include "XrdSysUtils.hh"
  
/******************************************************************************/
/*                              E x e c N a m e                               */
/******************************************************************************/

const char *XrdSysUtils::ExecName()
{
   static const char *myEname = 0;

// If we have been here before, simply return what we discovered. This is
// relatively thread-safe as we might loose some memory but it will work.
// Anyway, this method is unlikely to be called by multiple threads. Also,
// according to gthe Condor team, this code will not be able to return the
// program name if the program is under the control of valgrind!
//
   if (myEname) return myEname;

// Get the exec name based on platform
//
#ifdef __linux__
  {char epBuff[2048];
   int  epLen;
   if ((epLen = readlink("/proc/self/exe", epBuff, sizeof(epBuff)-1)) > 0)
      {epBuff[epLen] = 0;
       myEname = strdup(epBuff);
       return myEname;
      }
  }
#elif defined(__macos__)
  {char epBuff[2048];
   uint32_t epLen = sizeof(epBuff)-1;
   if (!_NSGetExecutablePath(epBuff, &epLen))
      {epBuff[epLen] = 0;
       myEname = strdup(epBuff);
       return myEname;
      }
  }
#elif defined(__solaris__)
  {const char *epBuff = getexecname();
   if (epBuff)
      {if (*epBuff == '/') myEname = strdup(epBuff);
          else {char *ename, *cwd = getcwd(0, MAXPATHLEN);
                ename = (char *)malloc(strlen(cwd)+1+strlen(epBuff)+1);
                sprintf(ename, "%s/%s", cwd, epBuff);
                myEname = ename;
                free(cwd);
               }
       return myEname;
      }
  }
#else
#endif

// If got here then we don't have a valid program name. Return a null string.
//
   return "";
}
