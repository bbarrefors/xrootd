
include( XRootDCommon )

#-------------------------------------------------------------------------------
# Shared library version
#-------------------------------------------------------------------------------
set( XRD_SEC_VERSION        0.0.1 )
set( XRD_SEC_SOVERSION      0 )
set( XRD_SEC_PWD_VERSION    0.0.1 )
set( XRD_SEC_PWD_SOVERSION  0 )
set( XRD_SEC_SSS_VERSION    0.0.1 )
set( XRD_SEC_SSS_SOVERSION  0 )
set( XRD_SEC_UNIX_VERSION   0.0.1 )
set( XRD_SEC_UNIX_SOVERSION 0 )

#-------------------------------------------------------------------------------
# The XrdSec library
#-------------------------------------------------------------------------------
add_library(
  XrdSec
  SHARED
  XrdSec/XrdSecClient.cc
                                      XrdSec/XrdSecEntity.hh
                                      XrdSec/XrdSecInterface.hh
  XrdSec/XrdSecPManager.cc            XrdSec/XrdSecPManager.hh
  XrdSec/XrdSecProtocolhost.cc        XrdSec/XrdSecProtocolhost.hh
  XrdSec/XrdSecServer.cc              XrdSec/XrdSecServer.hh
  XrdSec/XrdSecTLayer.cc              XrdSec/XrdSecTLayer.hh
  XrdSec/XrdSecTrace.hh )

target_link_libraries(
  XrdSec
  XrdUtils )

set_target_properties(
  XrdSec
  PROPERTIES
  VERSION   ${XRD_SEC_VERSION}
  SOVERSION ${XRD_SEC_SOVERSION} )

# FIXME: test
#-rw-r--r-- 1 ljanyst ljanyst  5806 2011-03-21 16:13 XrdSectestClient.cc
#-rw-r--r-- 1 ljanyst ljanyst 10758 2011-03-21 16:13 XrdSectestServer.cc

#-------------------------------------------------------------------------------
# The XrdSecpwd library
#-------------------------------------------------------------------------------
add_library(
  XrdSecpwd
  SHARED
  XrdSecpwd/XrdSecProtocolpwd.cc      XrdSecpwd/XrdSecProtocolpwd.hh
  XrdSecpwd/XrdSecpwdSrvAdmin.cc
                                      XrdSecpwd/XrdSecpwdPlatform.hh )

target_link_libraries(
  XrdSecpwd
  XrdUtils
  XrdCrypto
  ${CRYPT_LIBRARY} )

set_target_properties(
  XrdSecpwd
  PROPERTIES
  VERSION   ${XRD_SEC_PWD_VERSION}
  SOVERSION ${XRD_SEC_PWD_SOVERSION} )

#-------------------------------------------------------------------------------
# xrdpwdadmin
#-------------------------------------------------------------------------------
add_executable(
  xrdpwdadmin
  XrdSecpwd/XrdSecpwdSrvAdmin.cc )

target_link_libraries(
  xrdpwdadmin
  XrdSecpwd )

# FIXME: DONT_HAVE_CRYPT
# FIXME: linking against crypt

#-------------------------------------------------------------------------------
# The XrdSecsss library
#-------------------------------------------------------------------------------
add_library(
  XrdSecsss
  SHARED
  XrdSecsss/XrdSecProtocolsss.cc   XrdSecsss/XrdSecProtocolsss.hh
  XrdSecsss/XrdSecsssID.cc         XrdSecsss/XrdSecsssID.hh
  XrdSecsss/XrdSecsssKT.cc         XrdSecsss/XrdSecsssKT.hh
                                   XrdSecsss/XrdSecsssRR.hh )

target_link_libraries(
  XrdSecsss
  XrdUtils
  XrdCryptoLite )

set_target_properties(
  XrdSecsss
  PROPERTIES
  VERSION   ${XRD_SEC_SSS_VERSION}
  SOVERSION ${XRD_SEC_SSS_SOVERSION} )

#-------------------------------------------------------------------------------
# xrdsssadmin
#-------------------------------------------------------------------------------
add_executable(
  xrdsssadmin
  XrdSecsss/XrdSecsssAdmin.cc )

target_link_libraries(
  xrdsssadmin
  XrdSecsss
  XrdCryptoLite )

#-------------------------------------------------------------------------------
# The XrdSecunix library
#-------------------------------------------------------------------------------
add_library(
  XrdSecunix
  SHARED
  XrdSecunix/XrdSecProtocolunix.cc )

target_link_libraries(
  XrdSecunix
  XrdUtils )

set_target_properties(
  XrdSecunix
  PROPERTIES
  VERSION   ${XRD_SEC_UNIX_VERSION}
  SOVERSION ${XRD_SEC_UNIX_SOVERSION} )

#-------------------------------------------------------------------------------
# Install
#-------------------------------------------------------------------------------
install(
  TARGETS XrdSec XrdSecpwd XrdSecsss XrdSecunix xrdsssadmin xrdpwdadmin
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} )

install(
  FILES
  ${PROJECT_SOURCE_DIR}/docs/man/xrdsssadmin.8
  ${PROJECT_SOURCE_DIR}/docs/man/xrdpwdadmin.8
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man8 )

install(
  FILES
  XrdSec/XrdSecEntity.hh
  XrdSec/XrdSecInterface.hh
  XrdSec/XrdSecTLayer.hh
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/xrootd/XrdSec )

install(
  FILES
  XrdSecsss/XrdSecsssID.hh
  XrdSecsss/XrdSecsssKT.hh
  XrdSecsss/XrdSecsssRR.hh
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/xrootd/XrdSecsss )
