/******************************************************************************/
/*                                                                            */
/*                        X r d C m s S t a t e . c c                         */
/*                                                                            */
/* (c) 2007 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*                            All Rights Reserved                             */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC02-76-SFO0515 with the Department of Energy              */
/******************************************************************************/
  
//         $Id$

// Original Version: 1.3 2006/04/05 02:28:09 abh

const char *XrdCmsStateCVSID = "$Id$";

#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "XProtocol/YProtocol.hh"

#include "Xrd/XrdLink.hh"

#include "XrdCms/XrdCmsManager.hh"
#include "XrdCms/XrdCmsRTable.hh"
#include "XrdCms/XrdCmsState.hh"
#include "XrdCms/XrdCmsTrace.hh"

#include "XrdSys/XrdSysError.hh"

using namespace XrdCms;
 
/******************************************************************************/
/*                               G l o b a l s                                */
/******************************************************************************/
  
XrdCmsState XrdCms::CmsState;

/******************************************************************************/
/*                           C o n s t r u c t o r                            */
/******************************************************************************/
  
XrdCmsState::XrdCmsState() : mySemaphore(0)
{
   minNodeCnt   = 1;
   numActive    = 0;
   numStaging   = 0;
   currState    = All_NoStage | All_Suspend;
   prevState    = 0;
   Suspended    = 1;
   feOK         = 0;
   noSpace      = 0;
   adminNoStage = 0;
   adminSuspend = 0;
   NoStageFile  = "";
   SuspendFile  = "";
   Enabled      = 0;
}
 
/******************************************************************************/
/* Punlic                         E n a b l e                                 */
/******************************************************************************/
  
void XrdCmsState::Enable()
{
   EPNAME("Enable");
   struct stat buff;

// Set correct admin staging state
//
   Update(Stage, stat(NoStageFile, &buff));

// Set correct admin suspend state
//
   Update(Active, stat(SuspendFile, &buff));

// We will force the information to be sent to interested parties by making
// the previous state different from the current state and enabling ourselves.
//
   myMutex.Lock();
   Enabled = 1;
   prevState = ~currState;
   mySemaphore.Post();
   myMutex.UnLock();
}

/******************************************************************************/
/* Public                        M o n i t o r                                */
/******************************************************************************/
  
void *XrdCmsState::Monitor()
{
   EPNAME("Monitor");
   CmsStatusRequest myStatus = {{0, kYR_status, 0, 0}};
   int theState, Changes;

// Do this forever (we are only posted when finally enabled)
//
   do {mySemaphore.Wait();
       myMutex.Lock(); 
       Changes   = currState ^ prevState;
       theState  = currState;
       prevState = currState;
       myMutex.UnLock();

       if (Changes && (myStatus.Hdr.modifier = Status(Changes, theState)))
          {DEBUG("Sending status " <<(theState & Changes));
           RTable.Send((char *)&myStatus, sizeof(myStatus));
           Manager.Inform(myStatus.Hdr);
          }
      } while(1);

// All done
//
   return (void *)0;
}
  
/******************************************************************************/
/* Public                      s e n d S t a t e                              */
/******************************************************************************/
  
void XrdCmsState::sendState(XrdLink *lp)
{
   CmsStatusRequest myStatus = {{0, kYR_status, 0, 0}};

   myMutex.Lock();
   myStatus.Hdr.modifier  = Suspended
                          ? CmsStatusRequest::kYR_Suspend
                          : CmsStatusRequest::kYR_Resume;

   myStatus.Hdr.modifier |= NoStaging
                          ? CmsStatusRequest::kYR_noStage
                          : CmsStatusRequest::kYR_Stage;

   lp->Send((char *)&myStatus.Hdr, sizeof(myStatus.Hdr));
   myMutex.UnLock();
}

/******************************************************************************/
/* Public                            S e t                                    */
/******************************************************************************/
  
void XrdCmsState::Set(int ncount, const char *AdminPath)
{

// Set the node count (this requires a lock)
//
   myMutex.Lock(); 
   minNodeCnt = ncount;
   myMutex.UnLock();

// If we have an adminpath then this is an configuration call as well.
//
   if (AdminPath)
      {char fnbuff[1048];
       int i;

       i = strlen(AdminPath);
       strcpy(fnbuff, AdminPath);
       if (AdminPath[i-1] != '/') fnbuff[i++] = '/';
       strcpy(fnbuff+i, "NOSTAGE");
       NoStageFile = strdup(fnbuff);
       strcpy(fnbuff+i, "SUSPEND");
       SuspendFile = strdup(fnbuff);
      }
}

/******************************************************************************/
/* Private                        S t a t u s                                 */
/******************************************************************************/
  
unsigned char XrdCmsState::Status(int Changes, int theState)
{
   const char *SRstate = 0, *SNstate = 0;
   unsigned char rrModifier;

// Check for suspend changes
//
   if (Changes & All_Suspend)
      if (theState & All_Suspend)
         {rrModifier = CmsStatusRequest::kYR_Suspend;
          SRstate = "suspended";
         } else {
          rrModifier = CmsStatusRequest::kYR_Resume;
          SRstate = "active";
         }
      else rrModifier = 0;

// Check for staging changes
//
   if (Changes & All_NoStage)
      if (theState & All_NoStage)
         {rrModifier |= CmsStatusRequest::kYR_noStage;
          SNstate = "+ nostaging";
         } else {
          rrModifier |= CmsStatusRequest::kYR_Stage;
          SNstate = "+ staging";
         }

// Report and return status
//
   if (rrModifier) 
      {if (!SRstate && SNstate) SNstate += 2;
       Say.Emsg("State", "Status changed to", SRstate, SNstate);
      }
   return rrModifier;
}
 
/******************************************************************************/
/* Public                         U p d a t e                                 */
/******************************************************************************/

void XrdCmsState::Update(StateType StateT, int ActivCnt, int StageCnt)
{
  EPNAME("Update");
  char newVal;
  DEBUG("Type=" <<StateT <<" ActivCnt=" <<ActivCnt <<" StageCnt=" <<StageCnt);


// Create new state
//
   myMutex.Lock();
   switch(StateT)
         {case Active:   if ((newVal = ActivCnt ? 0 : 1) != adminSuspend)
                            {if (newVal) unlink(SuspendFile);
                                else close(open(SuspendFile, O_WRONLY|O_CREAT,
                                                             S_IRUSR|S_IWUSR));
                             adminSuspend = newVal;
                            }
                         break;
          case Counts:   numStaging += StageCnt;
                         numActive  += ActivCnt;
                         break;
          case FrontEnd: feOK = ActivCnt ? 1 : 0;
                         break;
          case Space:    noSpace = (ActivCnt ? 0 : 1);
                         break;
          case Stage:    if ((newVal = ActivCnt ? 0 : 1) != adminNoStage)
                            {if (newVal) unlink(NoStageFile);
                                else close(open(NoStageFile, O_WRONLY|O_CREAT,
                                                             S_IRUSR|S_IWUSR));
                             adminNoStage = newVal;
                            }
                         break;
          default:       Say.Emsg("State", "Invalid state update");
                         break;
         }

   currState=(numActive  < minNodeCnt ||adminSuspend|| !feOK  ? All_Suspend:0)
            |(numStaging < 1          ||adminNoStage|| noSpace? All_NoStage:0);

   Suspended = currState & All_Suspend;
   NoStaging = currState & All_NoStage;

// If any changes are noted then we must send out notifications
//
   if (currState != prevState && Enabled) mySemaphore.Post();

// All done
//
   myMutex.UnLock();
}