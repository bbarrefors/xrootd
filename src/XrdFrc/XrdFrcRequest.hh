#ifndef __FRCREQUEST_H__
#define __FRCREQUEST_H__
/******************************************************************************/
/*                                                                            */
/*                      X r d F r c R e q u e s t . h h                       */
/*                                                                            */
/* (c) 2010 by the Board of Trustees of the Leland Stanford, Jr., University  */
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

class XrdFrcRequest
{
public:

char      LFN[3072];    // Logical File Name ('\0' optional opaque)
char      User[256];    // User trace identifier
char      ID[40];       // Request ID
char      Notify[512];  // Notification path
char      Reserved[88];
char      iName[32];    // Instance name
char      csValue[64];  // Checksum value (dependent on csType).
long long addTOD;       // Time added to queue
int       This;         // Offset to this request
int       Next;         // Offset to next request
int       Options;      // Processing options (see below)
short     LFO;          // Offset to lfn in url if LFN is a url (o/w 0)
short     Opaque;       // Offset to '?' in LFN if exists, 0 o/w
char      Reserved2;
char      csType;       // Checksum type
char      OPc;          // Original Operation Request code (debugging)
char      Prty;         // Request priority

// Flags set in options
//
static const int msgFail  = 0x00000001;
static const int msgSucc  = 0x00000002;
static const int makeRW   = 0x00000004;
static const int Migrate  = 0x00000010;
static const int Purge    = 0x00000020;
static const int Register = 0x00000040;

// Checksum types (not all of which are supported)
//
static const int csNone   = 0;
static const int csSHA1   = 1;
static const int csSHA2   = 2;
static const int csSHA3   = 3;
static const int csAdler32= 4;
static const int csMD5    = 5;
static const int csCRC32  = 6;

// These define valid priorities
//
static const int maxPrty  = 2;
static const int maxPQE   = 3;

// The following define what can be listed from the queue
//
enum Item {getOBJ = 0, getLFN, getOBJCGI, getLFNCGI, getMODE, getNOTE, getOP,
           getPRTY,    getQWT, getRID,    getTOD,    getUSER, getLast};

// These define possible queues along with the "nil" queue
//
static const int     stgQ = 0;  // Stage    queue
static const int     migQ = 1;  // Migrate  queue
static const int     getQ = 2;  // Copy in  queue
static const int     putQ = 3;  // Copy out queue
static const int     nilQ = 4;  // Empty    queue
static const int     numQ = 5;
static const int     outQ = 1;  // Used as a mask only
};
#endif
