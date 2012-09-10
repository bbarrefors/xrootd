/******************************************************************************/
/*                                                                            */
/*                     X r d N e t S e c u r i t y . c c                      */
/*                                                                            */
/* (c) 2004 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC02-76-SFO0515 with the Department of Energy              */
/*                                                                            */
/* This file is part of the XRootD software suite.                            */
/*                                                                            */
/* XRootD is free software: you can redistribute it and/or modify it under    */
/* the terms of the GNU Lesser General Public License as published by the     */
/* Free Software Foundation, either version 3 of the License, or (at your     */
/* option) any later version.                                                 */
/*                                                                            */
/* XRootD is distributed in the hope that it will be useful, but WITHOUT      */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or      */
/* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public       */
/* License for more details.                                                  */
/*                                                                            */
/* You should have received a copy of the GNU Lesser General Public License   */
/* along with XRootD in a file called COPYING.LESSER (LGPL license) and file  */
/* COPYING (GPL license).  If not, see <http://www.gnu.org/licenses/>.        */
/*                                                                            */
/* The copyright holder's institutional names and contributor's names may not */
/* be used to endorse or promote products derived from this software without  */
/* specific prior written permission of the institution or contributor.       */
/******************************************************************************/

#ifndef WIN32
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <stdlib.h>
#include <sys/types.h>
#include <Winsock2.h>
#include <io.h>
int innetgr(const char *netgroup, const char *host, const char *user,
             const char *domain)
{
   return 0;
}
#include "XrdSys/XrdWin32.hh"
#endif

#include "XrdSys/XrdSysDNS.hh"
#include "XrdNet/XrdNetSecurity.hh"
#include "XrdOuc/XrdOucTrace.hh"

/******************************************************************************/
/*                         L o c a l   C l a s s e s                          */
/******************************************************************************/

class XrdNetTextList 
{
public:

XrdNetTextList *next; 
char           *text;

     XrdNetTextList(char *newtext) {next = 0; text = strdup(newtext);}
    ~XrdNetTextList() {if (text) free(text);}
};

/******************************************************************************/
/*                               D e f i n e s                                */
/******************************************************************************/
  
#define DEBUG(x) if (eTrace) {eTrace->Beg(TraceID); cerr <<x; eTrace->End();}

/******************************************************************************/
/*                               G l o b a l s                                */
/******************************************************************************/
  
const char *XrdNetSecurity::TraceID = "NetSecurity";

/******************************************************************************/
/*                               A d d H o s t                                */
/******************************************************************************/
  
void XrdNetSecurity::AddHost(char *hname)
{
   char *Hname = 0;

// If host is an ip address then do short circuit add otherwise do a full add
//
   if (isdigit(*hname) && (Hname = XrdSysDNS::getHostName(hname)))
      OKHosts.Add(hname, Hname, 0, Hash_dofree);
      else {XrdOucNList *nlp = new XrdOucNList(hname);
            HostList.Insert(nlp);
           }
   if (Hname) {DEBUG(hname <<" (" <<Hname <<") added to authorized hosts.");}
      else    {DEBUG(hname <<" added to authorized hosts.");}
}

/******************************************************************************/
/*                           A d d N e t G r o u p                            */
/******************************************************************************/

void XrdNetSecurity::AddNetGroup(char *gname)
{
  XrdNetTextList *tlp = new XrdNetTextList(gname);

// Add netgroup to list of valid ones
//
   tlp->next = NetGroups;
   NetGroups = tlp;

// All done
//
   DEBUG(gname <<" added to authorized netgroups.");
}

/******************************************************************************/
/*                             A u t h o r i z e                              */
/******************************************************************************/

char *XrdNetSecurity::Authorize(struct sockaddr *addr)
{
   struct sockaddr_in *ip = (struct sockaddr_in *)addr;
   char ipbuff[64], *hname;
   const char *ipname;
   XrdNetTextList *tlp;

// Convert IP address to characters (eventually,
//
   if (!(ipname = (char *)inet_ntop(ip->sin_family, (void *)&(ip->sin_addr),
       ipbuff, sizeof(ipbuff)))) return (char *)0;

// Check if we have seen this host before
//
   okHMutex.Lock();
   if ((hname = OKHosts.Find(ipname)))
      {okHMutex.UnLock(); return strdup(hname);}

// Get the hostname for this IP address
//
   if (!(hname = XrdSysDNS::getHostName(*addr))) hname = strdup(ipname);

// Check if this host is in the the appropriate netgroup, if any
//
   if ((tlp = NetGroups))
      do {if (innetgr(tlp->text, hname, 0, 0))
          return hostOK(hname, ipname, "netgroup");
         } while ((tlp = tlp->next));

// Plow through the specific host list to see if the host
//
   if (HostList.Find(hname)) return hostOK(hname, ipname, "host");

// Host is not authorized
//
   okHMutex.UnLock();
   DEBUG(hname <<" not authorized");
   free(hname);
   return 0;
}

/******************************************************************************/
/*                                 M e r g e                                  */
/******************************************************************************/
  
void XrdNetSecurity::Merge(XrdNetSecurity *srcp)
{
   XrdOucNList    *np;
   XrdNetTextList *sp, *tp;

// First merge in all of the host entries
//
   while((np = srcp->HostList.Pop())) HostList.Replace(np);

// Next merge the netgroup list
//
   while((sp = srcp->NetGroups))
        {tp = NetGroups; srcp->NetGroups = sp->next;
         while(tp) if (!strcmp(tp->text, sp->text)) break;
                      else tp = tp->next;
         if (tp) delete sp;
            else {sp->next  = NetGroups;
                  NetGroups = sp;
                 }
        }

// Delete the remnants of the source object
//
   delete srcp;
}

/******************************************************************************/
/*                       P r i v a t e   M e t h o d s                        */
/******************************************************************************/
/******************************************************************************/
/*                                h o s t O K                                 */
/******************************************************************************/
  
char *XrdNetSecurity::hostOK(char *hname, const char *ipname, const char *why)
{

// Add host to valid host table and return true. Note that the okHMutex must
// be locked upon entry and it will be unlocked upon exit.
//
   OKHosts.Add(ipname, strdup(hname), lifetime, Hash_dofree);
   okHMutex.UnLock();
   DEBUG(hname <<" authorized via " <<why);
   return hname;
}
