#ifndef __SEC_ENTITY_H__
#define __SEC_ENTITY_H__
/******************************************************************************/
/*                                                                            */
/*                       X r d S e c E n t i t y . h h                        */
/*                                                                            */
/* (c) 2005 by the Board of Trustees of the Leland Stanford, Jr., University  */
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

// This structure is returned during authentication. This is most relevant
// for client authentication unless mutual authentication has been implemented
// in which case the client can also authenticate the server. It is embeded
// in each protocol object to facilitate mutual authentication. Note that the
// destructor does nothing and it is the responsibility of the protocol object
// to delete the XrdSecEntity data members, if need be. This is because
// there can only be one destructor instance for the class and it is ambiguous
// as to which shared library definition should be used. Since protocol objects
// have unique class names, each one can have a private destructor avoiding
// platform specific run-time loader address resolution ecentricities. The OO
// "fix" for this problem would require protocols to define a derived private
// destructor for this object which is more hassle than it's worth.
//
#define XrdSecPROTOIDSIZE 8

class  XrdSecEntity
{
public:
         char   prot[XrdSecPROTOIDSIZE];  // Protocol used
         char   *name;                    // Entity's name
         char   *host;                    // Entity's host name
         char   *vorg;                    // Entity's virtual organization
         char   *role;                    // Entity's role
         char   *grps;                    // Entity's group names
         char   *endorsements;            // Protocol specific endorsements
         char   *creds;                   // Raw client credentials or certificate
         int     credslen;                // Length of the 'cert' field
         char   *moninfo;                 // Additional information for monitoring 
         char   *tident;                  // Trace identifier (do not touch)

         XrdSecEntity(const char *pName = "")
                        {strncpy(prot, pName, XrdSecPROTOIDSIZE-1);
                         prot[XrdSecPROTOIDSIZE-1] = '\0';
                         name=host=vorg=role=grps=endorsements=creds=moninfo=tident = 0;
                         credslen = 0;
                        }
        ~XrdSecEntity() {}
};

#define XrdSecClientName XrdSecEntity
#define XrdSecServerName XrdSecEntity
#endif
