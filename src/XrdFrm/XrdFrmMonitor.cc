/******************************************************************************/
/*                                                                            */
/*                      X r d F r m M o n i t o r . c c                       */
/*                                                                            */
/* (c) 2010 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*                            All Rights Reserved                             */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC03-76-SFO0515 with the Department of Energy              */
/******************************************************************************/

#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "XrdFrc/XrdFrcTrace.hh"
#include "XrdFrm/XrdFrmMonitor.hh"
#include "XrdNet/XrdNet.hh"
#include "XrdNet/XrdNetPeer.hh"
#include "XrdOuc/XrdOucUtils.hh"
#include "XrdSys/XrdSysDNS.hh"
#include "XrdSys/XrdSysError.hh"
#include "XrdSys/XrdSysPlatform.hh"
#include "XrdSys/XrdSysPthread.hh"
#include "XrdSys/XrdSysTimer.hh"

using namespace XrdFrc;

/******************************************************************************/
/*                     S t a t i c   A l l o c a t i o n                      */
/******************************************************************************/
  
char              *XrdFrmMonitor::Dest1      = 0;
int                XrdFrmMonitor::monFD1     = -1;
int                XrdFrmMonitor::monMode1   = 0;
struct sockaddr    XrdFrmMonitor::InetAddr1;
char              *XrdFrmMonitor::Dest2      = 0;
int                XrdFrmMonitor::monFD2     = -1;
int                XrdFrmMonitor::monMode2   = 0;
struct sockaddr    XrdFrmMonitor::InetAddr2;
kXR_int32          XrdFrmMonitor::startTime  = 0;
int                XrdFrmMonitor::isEnabled  = 0;
char              *XrdFrmMonitor::idRec      = 0;
int                XrdFrmMonitor::idLen      = 0;
int                XrdFrmMonitor::sidSize    = 0;
char              *XrdFrmMonitor::sidName    = 0;
int                XrdFrmMonitor::idTime     = 3600;
char               XrdFrmMonitor::monMIGR    = 0;
char               XrdFrmMonitor::monPURGE   = 0;
char               XrdFrmMonitor::monSTAGE   = 0;

/******************************************************************************/
/*                     T h r e a d   I n t e r f a c e s                      */
/******************************************************************************/

void *XrdFrmMonitorID(void *parg)
{
   XrdFrmMonitor::Ident();
   return (void *)0;
}
  
/******************************************************************************/
/*                              D e f a u l t s                               */
/******************************************************************************/

void XrdFrmMonitor::Defaults(char *dest1, int mode1, char *dest2, int mode2,
                             int iTime)
{

// Make sure if we have a proper destinations and modes
//
   if (dest1 && !mode1) {free(dest1); dest1 = 0; mode1 = 0;}
   if (dest2 && !mode2) {free(dest2); dest2 = 0; mode2 = 0;}

// Propogate the destinations
//
   if (!dest1)
      {mode1 = (dest1 = dest2) ? mode2 : 0;
       dest2 = 0; mode2 = 0;
      }

// Set the default destinations (caller supplied strdup'd strings)
//
   if (Dest1) free(Dest1);
   Dest1 = dest1; monMode1 = mode1;
   if (Dest2) free(Dest2);
   Dest2 = dest2; monMode2 = mode2;

// Set overall monitor mode
//
   monMIGR   = ((mode1 | mode2) & XROOTD_MON_MIGR  ? 1 : 0);
   monPURGE  = ((mode1 | mode2) & XROOTD_MON_PURGE ? 1 : 0);
   monSTAGE  = ((mode1 | mode2) & XROOTD_MON_STAGE ? 1 : 0);

// Do final check
//
   isEnabled = (Dest1 == 0 && Dest2 == 0 ? 0 : 1);
   idTime = iTime;
}

/******************************************************************************/
/*                                 I d e n t                                  */
/******************************************************************************/
  
void XrdFrmMonitor::Ident()
{
do{Send(-1, idRec, idLen);
   XrdSysTimer::Snooze(idTime);
  } while(1);
}

/******************************************************************************/
/*                                  I n i t                                   */
/******************************************************************************/
  
int XrdFrmMonitor::Init(const char *iHost, const char *iProg, const char *iName)
{
   XrdNet     myNetwork(&Say, 0);
   XrdNetPeer monDest;
   XrdXrootdMonMap *mP;
   long long  mySid;
   char      *etext, iBuff[1024], *sName;

// Generate our server ID
//
   sidName = XrdOucUtils::Ident(mySid, iBuff, sizeof(iBuff), iHost, iProg,
                                (iName ? iName : "anon"), 0);
   sidSize = strlen(sidName);
   startTime = htonl(time(0));

// There is nothing to do unless we have been enabled via Defaults()
//
   if (!isEnabled) return 1;

// Create identification record
//
   idLen = strlen(iBuff) + sizeof(XrdXrootdMonHeader) + sizeof(kXR_int32);
   idRec = (char *)malloc(idLen+1);
   mP = (XrdXrootdMonMap *)idRec;
   fillHeader(&(mP->hdr), XROOTD_MON_MAPIDNT, idLen);
   mP->hdr.pseq = 0;
   mP->dictid   = 0;
   strcpy(mP->info, iBuff);

// Get the address of the primary destination
//
   if (!XrdSysDNS::Host2Dest(Dest1, InetAddr1, &etext))
      {Say.Emsg("Monitor", "setup monitor collector;", etext);
       return 0;
      }

// Allocate a socket for the primary destination
//
   if (!myNetwork.Relay(monDest, Dest1, XRDNET_SENDONLY)) return 0;
   monFD1 = monDest.fd;

// Do the same for the secondary destination
//
   if (Dest2)
      {if (!XrdSysDNS::Host2Dest(Dest2, InetAddr2, &etext))
          {Say.Emsg("Monitor", "setup monitor collector;", etext);
           return 0;
          }
       if (!myNetwork.Relay(monDest, Dest2, XRDNET_SENDONLY)) return 0;
       monFD2 = monDest.fd;
      }

// Check if we will be producing identification records
//
   if (idTime)
      {pthread_t tid;
       int retc;
       if ((retc = XrdSysThread::Run(&tid,XrdFrmMonitorID,0,0,"mon ident")))
          {Say.Emsg("Init", retc, "create monitor ident thread"); return 0;}
      }

// All done
//
   return 1;
}

/******************************************************************************/
/*                                   M a p                                    */
/******************************************************************************/
  
kXR_unt32 XrdFrmMonitor::Map(char code, const char *uname, const char *path)
{
   XrdXrootdMonMap     map;
   const char *colonP, *atP;
   char uBuff[1024];
   int  size, montype;

// Decode the user name as a.b:c@d
//
   if ((colonP = index(uname, ':')) && (atP = index(colonP+1, '@')))
      {int n = colonP - uname + 1;
       strncpy(uBuff, uname, n);
       strcpy(uBuff+n, sidName);
       strcpy(uBuff+n+sidSize, atP);
      } else strcpy(uBuff, uname);

// Copy in the username and path the dictid is always zero for us.
//
   map.dictid = 0;
   strcpy(map.info, uBuff);
   size = strlen(uBuff);
   if (path)
      {*(map.info+size) = '\n';
       strlcpy(map.info+size+1, path, sizeof(map.info)-size-1);
       size = size + strlen(path) + 1;
      }

// Route the packet to all destinations that need them
//
        if (code == XROOTD_MON_MAPSTAG){montype = XROOTD_MON_STAGE;
                                        code    = XROOTD_MON_MAPXFER;
                                       }
   else if (code == XROOTD_MON_MAPMIGR){montype = XROOTD_MON_MIGR;
                                        code    = XROOTD_MON_MAPXFER;
                                       }
   else if (code == XROOTD_MON_MAPPURG) montype = XROOTD_MON_PURGE;
   else                                 montype = XROOTD_MON_INFO;

// Fill in the header and route the packet
//
   size = sizeof(XrdXrootdMonHeader)+sizeof(kXR_int32)+size;
   fillHeader(&map.hdr, code, size);
// cerr <<"Mon send "<<code <<": " <<map.info <<endl;
   Send(montype, (void *)&map, size);

// Return the dictionary id
//
   return map.dictid;
}

/******************************************************************************/
/*                       P r i v a t e   M e t h o d s                        */
/******************************************************************************/
/******************************************************************************/
/*                            f i l l H e a d e r                             */
/******************************************************************************/
  
void XrdFrmMonitor::fillHeader(XrdXrootdMonHeader *hdr,
                               const char id, int size)
{  static XrdSysMutex seqMutex;
   static int         seq = 0;
          int         myseq;

// Generate a new sequence number
//
   seqMutex.Lock();
   myseq = 0x00ff & (seq++);
   seqMutex.UnLock();

// Fill in the header
//
   hdr->code = static_cast<kXR_char>(id);
   hdr->pseq = static_cast<kXR_char>(myseq);
   hdr->plen = htons(static_cast<uint16_t>(size));
   hdr->stod = startTime;
}
 
/******************************************************************************/
/*                                  S e n d                                   */
/******************************************************************************/
  
int XrdFrmMonitor::Send(int monMode, void *buff, int blen)
{
    EPNAME("Send");
    static XrdSysMutex sendMutex;
    int rc1, rc2;
    sendMutex.Lock();
    if (monMode & monMode1 && monFD1 >= 0)
       {rc1  = (int)sendto(monFD1, buff, blen, 0,
                        (const struct sockaddr *)&InetAddr1, sizeof(sockaddr));
        DEBUG(blen <<" bytes sent to " <<Dest1 <<" rc=" <<(rc1 ? errno : 0));
       }
       else rc1 = 0;
    if (monMode & monMode2 && monFD2 >= 0)
       {rc2 = (int)sendto(monFD2, buff, blen, 0,
                        (const struct sockaddr *)&InetAddr2, sizeof(sockaddr));
        DEBUG(blen <<" bytes sent to " <<Dest2 <<" rc=" <<(rc2 ? errno : 0));
       }
       else rc2 = 0;
    sendMutex.UnLock();

    return (rc1 > rc2 ? rc1 : rc2);
}
