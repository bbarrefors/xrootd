#ifndef __CMS_PREPARGS__H
#define __CMS_PREPARGS__H
/******************************************************************************/
/*                                                                            */
/*                     X r d C m s P r e p A r g s . h h                      */
/*                                                                            */
/* (c) 2007 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*                            All Rights Reserved                             */
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
  
#include "XProtocol/YProtocol.hh"

#include "Xrd/XrdJob.hh"
#include "XrdCms/XrdCmsNode.hh"
#include "XrdCms/XrdCmsRRData.hh"
#include "XrdSys/XrdSysPthread.hh"

class XrdCmsPrepArgs : public XrdJob
{
public:
static const int       iovNum = 2;

XrdCms::CmsRRHdr        Request;
        char           *Ident;
        char           *reqid;
        char           *notify;
        char           *prty;
        char           *mode;
        char           *path;
        const char     *opaque;
        char           *clPath;   // ->coloc path, if any
        int             options;
        int             pathlen;  // Includes null byte

        struct iovec    ioV[iovNum];  // To forward the request

        void            DoIt() {if (!XrdCmsNode::do_SelPrep(*this)) delete this;}

static  void            Process();

        void            Queue();

static  XrdCmsPrepArgs *getRequest();

                        XrdCmsPrepArgs(XrdCmsRRData &Arg);

                       ~XrdCmsPrepArgs() {if (Data) free(Data);}

private:

static XrdSysMutex      PAQueue;
static XrdSysSemaphore  PAReady;
       XrdCmsPrepArgs  *Next;
static XrdCmsPrepArgs  *First;
static XrdCmsPrepArgs  *Last;
static int              isIdle;
       char            *Data;

};
#endif
