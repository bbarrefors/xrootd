#ifndef __CMS_NODE__H
#define __CMS_NODE__H
/******************************************************************************/
/*                                                                            */
/*                         X r d C m s N o d e . h h                          */
/*                                                                            */
/* (c) 2007 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*                            All Rights Reserved                             */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC02-76-SFO0515 with the Department of Energy              */
/******************************************************************************/

//         $Id$

#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
  
#include "Xrd/XrdLink.hh"
#include "XrdCms/XrdCmsTypes.hh"
#include "XrdCms/XrdCmsRRQ.hh"
#include "XrdSys/XrdSysPthread.hh"

class XrdCmsDrop;
class XrdCmsPrepArgs;
class XrdCmsRRData;

class XrdCmsNode
{
friend class XrdCmsCluster;
public:
       char  *Ident;        // -> role hostname
       char   isDisable;    //0 Set via admin command to temporarily remove node
       char   isOffline;    //1 Set when a link failure occurs
       char   isNoStage;    //2 Set upon a nostage event
       char   isMan;        //3 Set when node acts as manager
       char   isPeer;       //4 Set when node acts as peer manager
       char   isProxy;      //5 Set when node acts as a proxy
       char   isSuspend;    //6 Set upon a suspend event
       char   isBound;      //7 Set when node is in the configuration
       char   isRW;         //0 Set when node can write or stage data
       char   isKnown;      //1 Set when we have recieved a "state"
       char   isConn;       //2 Set when node is network connected
       char   isGone;       //3 Set when node must be deleted
       char   isPerm;       //4 Set when node is permanently bound
       char   PingPong;     //5 Keep alive field
       char   isReserved[2];

       int    DiskFree;     // Largest free KB
       int    DiskUtil;     // Total disk utilization
       int    DiskNums;     // Number of file systems
unsigned int  ConfigID;     // Configuration identifier

const  char  *do_AvKb(XrdCmsRRData &Arg);
const  char  *do_Chmod(XrdCmsRRData &Arg);
const  char  *do_Disc(XrdCmsRRData &Arg);
const  char  *do_Gone(XrdCmsRRData &Arg);
const  char  *do_Have(XrdCmsRRData &Arg);
const  char  *do_Load(XrdCmsRRData &Arg);
const  char  *do_Locate(XrdCmsRRData &Arg);
static int    do_LocFmt(char *buff, XrdCmsSelected *sP, SMask_t pf, SMask_t wf);
const  char  *do_Mkdir(XrdCmsRRData &Arg);
const  char  *do_Mkpath(XrdCmsRRData &Arg);
const  char  *do_Mv(XrdCmsRRData &Arg);
const  char  *do_Ping(XrdCmsRRData &Arg);
const  char  *do_Pong(XrdCmsRRData &Arg);
const  char  *do_PrepAdd(XrdCmsRRData &Arg);
const  char  *do_PrepDel(XrdCmsRRData &Arg);
const  char  *do_Rm(XrdCmsRRData &Arg);
const  char  *do_Rmdir(XrdCmsRRData &Arg);
const  char  *do_Select(XrdCmsRRData &Arg);
static int    do_SelPrep(XrdCmsPrepArgs &Arg);
const  char  *do_Space(XrdCmsRRData &Arg);
const  char  *do_State(XrdCmsRRData &Arg);
       int    do_StateFWD(XrdCmsRRData &Arg);
const  char  *do_Stats(XrdCmsRRData &Arg);
const  char  *do_Status(XrdCmsRRData &Arg);
const  char  *do_Try(XrdCmsRRData &Arg);
const  char  *do_Update(XrdCmsRRData &Arg);
const  char  *do_Usage(XrdCmsRRData &Arg);

       void   Disc(const char *reason=0, int needLock=1);

inline int    ID(int &INum) {INum = Instance; return NodeID;}

inline int    Inst() {return Instance;}

inline int    isNode(SMask_t smask) {return (smask & NodeMask) != 0;}
inline int    isNode(const char *hn)
                    {return Link && !strcmp(Link->Host(), hn);}
inline int    isNode(unsigned int ipa)
                    {return ipa == IPAddr;}
inline int    isNode(unsigned int ipa, int port, const char *nid)
                    {return ipa == IPAddr && port == Port
                         && (nid ? !strcmp(myNID, nid) : 1);
                    }
inline char  *Name()   {return (myName ? myName : (char *)"?");}

inline char  *Name(int &len, int &port)
                       {len = myNlen; port = Port; return myName;}

inline SMask_t Mask() {return NodeMask;}

inline void    Lock() {myMutex.Lock(); isLocked = 1;}
inline void  UnLock() {isLocked = 0; myMutex.UnLock();}

static void  Report_Usage(XrdLink *lp);

inline int   Send(const char *buff, int blen=0)
                 {return (isOffline ? -1 : Link->Send(buff, blen));}
inline int   Send(const struct iovec *iov, int iovcnt, int iotot=0)
                 {return (isOffline ? -1 : Link->Send(iov, iovcnt, iotot));}

       void  setName(const char *Role, XrdLink *lnkp, int port);

inline void  setSlot(short rslot) {RSlot = rslot;}
inline short getSlot() {return RSlot;}

static void  setSpace(int Dfree, int Dutil) {dsk_free = Dfree; dsk_util = Dutil;}

       void  SyncSpace();

             XrdCmsNode(const char *Role,  XrdLink *lnkp, int port=0,
                        const char *sid=0, int lvl=0, int id=-1);
            ~XrdCmsNode();

private:
       int   getMode(const char *theMode, mode_t &Mode);
static int   isOnline(char *path, int upt=1);

XrdSysMutex        myMutex;
XrdLink           *Link;
unsigned int       IPAddr;
XrdCmsNode        *Next;
time_t             DropTime;
XrdCmsDrop        *DropJob;  
int                IPV6Len;  // 12345678901234567890123456
char               IPV6[28]; // [::123.123.123.123]:123456

SMask_t            NodeMask;
int                NodeID;
int                Instance;
int                Port;
int                myLevel;
char              *myNID;
char              *myName;
int                myNlen;

int                logload;
int                myCost;       // Overall cost (determined by location)
int                myLoad;       // Overall load
int                myMass;       // Overall load including space utilization
int                RefA;         // Number of times used for allocation
int                RefTotA;
int                RefR;         // Number of times used for redirection
int                RefTotR;
short              RSlot;
char               isLocked;
char               RSVD;

// The following fields are used to keep the supervisor's load values
//
static XrdSysMutex mlMutex;
static int         xeq_load;
static int         cpu_load;
static int         mem_load;
static int         pag_load;
static int         net_load;
static int         dsk_free;
static int         dsk_util;
};
#endif