/**************************************************************************************/
/*                                                                            */
/*                            X r d M a i n . c c                             */
/*                                                                            */
/* (c) 2004 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*       All Rights Reserved. See XrdInfo.cc for complete License Terms       */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC03-76-SFO0515 with the Department of Energy              */
/******************************************************************************/

/* This is the XRootd server. The syntax is:

   xrootd [options]

   options: [-b] [-c <fname>] [-d] [-h] [-l <fname>] [-p <port>] [<oth>]

Where:
   -b     forces background execution.

   -c     specifies the configuration file. This may also come from the
          XrdCONFIGFN environmental variable.

   -d     Turns on debugging mode (equivalent to xrd.trace all)

   -h     Displays usage line and exits.

   -l     Specifies location of the log file. This may also come from the
          XrdOucLOGFILE environmental variable or from the oofs layer. By
          By default, error messages go to standard error.

   -p     Is the port to use either as a service name or an actual port number.
          The default port is 1094.

   <oth>  Are other protocol specific options.

*/

/******************************************************************************/
/*                         i n c l u d e   f i l e s                          */
/******************************************************************************/
  
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <sys/param.h>

#include "Xrd/XrdConfig.hh"
#include "Xrd/XrdInet.hh"
#include "Xrd/XrdLink.hh"
#include "Xrd/XrdProtLoad.hh"
#include "Xrd/XrdScheduler.hh"

#include "XrdSys/XrdSysError.hh"
#include "XrdSys/XrdSysHeaders.hh"
#include "XrdSys/XrdSysPthread.hh"

/******************************************************************************/
/*                         L o c a l   C l a s s e s                          */
/******************************************************************************/
  
class XrdMain
{
public:

XrdInet             *theNet;
int                  thePort;
static XrdConfig     Config;

           XrdMain() {}
           XrdMain(XrdInet *nP) : theNet(nP), thePort(nP->Port()) {}
          ~XrdMain() {}
};

XrdConfig XrdMain::Config;

/******************************************************************************/
/*            E x t e r n a l   T h r e a d   I n t e r f a c e s             */
/******************************************************************************/
  
void *mainAccept(void *parg)
{  XrdMain      *Parms   = (XrdMain *)parg;
   XrdInet      *myNet   =  Parms->theNet;
   XrdScheduler *mySched =  Parms->Config.ProtInfo.Sched;
   XrdProtLoad   ProtSelect(Parms->thePort);
   XrdLink      *newlink;

   while(1) if ((newlink = myNet->Accept(XRDNET_NODNTRIM)))
               {newlink->setProtocol((XrdProtocol *)&ProtSelect);
                mySched->Schedule((XrdJob *)newlink);
               }

   return (void *)0;
}

/******************************************************************************/
/*                             m a i n A d m i n                              */
/******************************************************************************/
  
void *mainAdmin(void *parg)
{  XrdMain      *Parms   = (XrdMain *)parg;
   XrdInet      *NetADM  =  Parms->theNet;
   XrdLink      *newlink;
// static XrdProtocol_Admin  ProtAdmin;
   int           ProtAdmin;

// At this point we should be able to accept new connections. Noe that we don't
// support admin connections as of yet so the following code is superflous.
//
   while(1) if ((newlink = NetADM->Accept()))
               {newlink->setProtocol((XrdProtocol *)&ProtAdmin);
                Parms->Config.ProtInfo.Sched->Schedule((XrdJob *)newlink);
               }
   return (void *)0;
}

/******************************************************************************/
/*                                  m a i n                                   */
/******************************************************************************/
  
int main(int argc, char *argv[])
{
   XrdMain   Main;
   sigset_t  myset;
   pthread_t tid;
   char      buff[128];
   int       i, retc;

// Turn off sigpipe and host a variety of others before we start any threads
//
   signal(SIGPIPE, SIG_IGN);  // Solaris optimization
   sigemptyset(&myset);
   sigaddset(&myset, SIGPIPE);
   sigaddset(&myset, SIGUSR1);
   sigaddset(&myset, SIGUSR2);
#ifdef SIGRTMAX
   sigaddset(&myset, SIGRTMAX);
   sigaddset(&myset, SIGRTMAX-1);
#endif
   sigaddset(&myset, SIGCHLD);
   pthread_sigmask(SIG_BLOCK, &myset, NULL);

// Set the default stack size here
//
   if (sizeof(long) > 4) XrdSysThread::setStackSize((size_t)1048576);
      else               XrdSysThread::setStackSize((size_t)786432);

// Process configuration file
//
   if (Main.Config.Configure(argc, argv)) _exit(1);

// Start the admin thread if an admin network is defined
//
   if (Main.Config.NetADM && (retc = XrdSysThread::Run(&tid, mainAdmin,
                             (void *)new XrdMain(Main.Config.NetADM),
                             XRDSYSTHREAD_BIND, "Admin handler")))
      {Main.Config.ProtInfo.eDest->Emsg("main", retc, "create admin thread");
       _exit(3);
      }

// At this point we should be able to accept new connections. Spawn a
// thread for each network except the first. The main thread will handle
// that network as some implementations require a main active thread.
//
   for (i = 1; i <= XrdProtLoad::ProtoMax; i++)
       if (Main.Config.NetTCP[i])
          {XrdMain *Parms = new XrdMain(Main.Config.NetTCP[i]);
           sprintf(buff, "Port %d handler", Parms->thePort);
           if (Parms->theNet == Main.Config.NetTCP[XrdProtLoad::ProtoMax])
               Parms->thePort = -(Parms->thePort);
           if ((retc = XrdSysThread::Run(&tid, mainAccept, (void *)Parms,
                                         XRDSYSTHREAD_BIND, strdup(buff))))
              {Main.Config.ProtInfo.eDest->Emsg("main", retc, "create", buff);
               _exit(3);
              }
          }

// Finally, start accepting connections on the main port
//
   Main.theNet  = Main.Config.NetTCP[0];
   Main.thePort = Main.Config.NetTCP[0]->Port();
   mainAccept((void *)&Main);

// We should never get here
//
   pthread_exit(0);
}
