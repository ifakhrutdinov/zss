

/*
  This program and the accompanying materials are
  made available under the terms of the Eclipse Public License v2.0 which accompanies
  this distribution, and is available at https://www.eclipse.org/legal/epl-v20.html
  
  SPDX-License-Identifier: EPL-2.0
  
  Copyright Contributors to the Zowe Project.
*/

#ifdef METTLE
#include <metal/metal.h>
#include <metal/stddef.h>
#include <metal/stdlib.h>
#include <metal/string.h>
#else
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#endif

#include "zowetypes.h"
#include "cmutils.h"
#include "crossmemory.h"
#include "zis/client.h"
#include "zis/service.h"
#include "zis/server.h"

CrossMemoryServerName zisGetDefaultServerName() {
  return CMS_DEFAULT_SERVER_NAME;
}

const char *ZIS_COPY_RC_DESCRIPTION[] = {
  [RC_ZIS_SNRFSRV_OK] = "Ok",
  [RC_ZIS_SNRFSRV_PARMLIST_NULL] = "Parm list is null",
  [RC_ZIS_SNRFSRV_BAD_EYECATCHER] = "Bad eyecatcher",
  [RC_ZIS_SNRFSRV_BAD_DEST] = "Bad destination",
  [RC_ZIS_SNRFSRV_BAD_ASCB] = "Bad ASCB",
  [RC_ZIS_SNRFSRV_BAD_SIZE] = "Bad size",
  [RC_ZIS_SNRFSRV_ECSA_ALLOC_FAILED] = "ESCA allocation failed",
  [RC_ZIS_SNRFSRV_RECOVERY_ERROR] = "Recovery error",
  [RC_ZIS_SNRFSRV_SRC_ASSB_ABEND] = "ASSB abended",
  [RC_ZIS_SNRFSRV_SRC_IEAMSCHD_ABEND] = "IEAMSCHD abended",
  [RC_ZIS_SNRFSRV_SRC_IEAMSCHD_FAILED] = "IEAMSCHD failed",
  [RC_ZIS_SNRFSRV_SHARED_OBJ_ALLOC_FAILED] = "Shared object allocation failed",
  [RC_ZIS_SNRFSRV_SHARED_OBJ_SHARE_FAILED] = "Shared object sharing failed",
  [RC_ZIS_SNRFSRV_SHARED_OBJ_DETACH_FAILED] = "Shared object detach failed "
};


const char *ZIS_AUTH_RC_DESCRIPTION[] = {
  [RC_ZIS_AUTHSRV_OK] = "Ok",
  [RC_ZIS_AUTHSRV_PARMLIST_NULL] = "Parm list is null",
  [RC_ZIS_AUTHSRV_BAD_EYECATCHER] = "Bad eyecatcher",
  [RC_ZIS_AUTHSRV_DELETE_FAILED] = "Deleting ACEE failed",
  [RC_ZIS_AUTHSRV_CREATE_FAILED] = "Creating ACEE failed",
  [RC_ZIS_AUTHSRV_UNKNOWN_FUNCTION_CODE] = "Unknown function code",
  [RC_ZIS_AUTHSRV_INPUT_STRING_TOO_LONG] = "Input string too long",
  [RC_ZIS_AUTHSRV_INSTALL_RECOVERY_FAILED] = "Installing recovery failed",
  [RC_ZIS_AUTHSRV_SAF_ABENDED] = "SAF abended",
  [RC_ZIS_AUTHSRV_SAF_ERROR] = "SAF error",
  [RC_ZIS_AUTHSRV_SAF_NO_DECISION] = "No SAF decision",
  [RC_ZIS_AUTHSRV_USER_CLASS_NOT_READ] = CMS_PROD_ID" class read failed",
  [RC_ZIS_AUTHSRV_USER_CLASS_TOO_LONG] = CMS_PROD_ID" class is too long",
};

int zisCopyDataFromAddressSpace(const CrossMemoryServerName *serverName,
                                void *dest, void *src, unsigned int size,
                                int srcKey, ASCB *ascb,
                                ZISCopyServiceStatus *status) {

  SnarferServiceParmList parmList = {
      .eyecatcher = ZIS_SNARFER_SERVICE_PARMLIST_EYECATCHER,
      .destinationAddress = (uint64)dest,
      .sourceAddress = (uint64)src,
      .sourceASCB = ascb,
      .sourceKey = srcKey,
      .size = size
  };
  int rc = RC_ZIS_SRVC_OK;

  /* since AMODE31 callers can assume that it's safe to pass addresses
   * with the 32nd bit on, we need to take care of that, because our code
   * operates in AMODE64 */
#ifndef _LP64
  parmList.destinationAddress &= 0x000000007FFFFFFFLLU;
  parmList.sourceAddress &= 0x000000007FFFFFFFLLU;
#endif

  const CrossMemoryServerName *localServerName = serverName ? serverName : &CMS_DEFAULT_SERVER_NAME;

  int serviceRC = 0;
  int cmsRC = cmsCallService(localServerName, ZIS_SERVICE_ID_SNARFER_SRV, &parmList, &serviceRC);

  if (cmsRC != RC_CMS_OK) {
    rc = RC_ZIS_SRVC_CMS_FAILED;
    status->baseStatus.cmsRC = cmsRC;
  } else if (serviceRC != RC_ZIS_SNRFSRV_OK) {
    rc = RC_ZIS_SRVC_SERVICE_FAILED;
    status->baseStatus.serviceRC = serviceRC;
  }

  return rc;
}

static int zisCallServiceInternal(const CrossMemoryServerName *serverName,
                                  const ZISServicePath *path, void *parm,
                                  unsigned int version,
                                  bool noSAFCheck,
                                  ZISServiceStatus *status) {

  if (status == NULL) {
    return RC_ZIS_SRVC_STATUS_NULL;
  }

  CrossMemoryServerGlobalArea *cmsGA = NULL;
  int getGlobalAreaRC = cmsGetGlobalArea(serverName, &cmsGA);
  if (getGlobalAreaRC != RC_CMS_OK) {
    return RC_ZIS_SRVC_GLOBAL_AREA_NULL;
  }

  if (!(cmsGA->serverFlags & CROSS_MEMORY_SERVER_FLAG_READY)) {
    status->cmsRC = RC_CMS_SERVER_NOT_READY;
    return RC_ZIS_SRVC_CMS_FAILED;
  }

 ZISServerAnchor *serverAnchor = cmsGA->userServerAnchor;
 if (serverAnchor == NULL) {
   return RC_ZIS_SRVC_SEVER_ANCHOR_NULL;
 }

 if (serverAnchor->serviceTable == NULL) {
   return RC_ZIS_SRVC_SERVICE_TABLE_NULL;
 }

 ZISServiceAnchor *serviceAnchor =
     crossMemoryMapGet(serverAnchor->serviceTable, path);
  if (serviceAnchor == NULL) {
    return RC_ZIS_SRVC_SERVICE_NOT_FOUND;
  }

  int routerServiceID = -1;
  if (serviceAnchor->flags & ZIS_SERVICE_ANCHOR_FLAG_SPACE_SWITCH) {
    routerServiceID = ZIS_SERVICE_ID_SRVC_ROUTER_SS;
  } else {
    routerServiceID = ZIS_SERVICE_ID_SRVC_ROUTER_CP;
  }

  ZISServiceRouterParm routerParmList = {0};
  memcpy(routerParmList.eyecatcher, ZIS_SERVICE_ROUTER_EYECATCHER,
         sizeof(routerParmList.eyecatcher));
  routerParmList.version = ZIS_SERVICE_ROUTER_VERSION;
  routerParmList.size = sizeof(ZISServiceRouterParm);
  routerParmList.targetServicePath = *path;
  routerParmList.targetServiceParm = parm;
  routerParmList.serviceVersion = version;

  int cmsFlags = CMS_CALL_FLAG_NONE;
  if (noSAFCheck) {
    cmsFlags = CMS_CALL_FLAG_NO_SAF_CHECK;
  }

  int routerRC = 0;
  int cmsRC = cmsCallService3(cmsGA, routerServiceID, &routerParmList,
                              cmsFlags, &routerRC);

  if (cmsRC != RC_CMS_OK) {
    status->cmsRC = cmsRC;
    return RC_ZIS_SRVC_CMS_FAILED;
  }

  if (routerRC != RC_ZIS_SRVC_OK) {
    status->serviceRC = routerRC;
    return RC_ZIS_SRVC_SERVICE_FAILED;
  }

  return RC_ZIS_SRVC_OK;
}

int zisCallService(const CrossMemoryServerName *serverName,
                   const ZISServicePath *path, void *parm,
                   ZISServiceStatus *status) {

  return zisCallServiceInternal(serverName, path, parm,
                                ZIS_SERVICE_ANY_VERSION,
                                false,
                                status);

}

int zisCallServiceUnchecked(const CrossMemoryServerName *serverName,
                            const ZISServicePath *path, void *parm,
                            ZISServiceStatus *status) {

  return zisCallServiceInternal(serverName, path, parm,
                                ZIS_SERVICE_ANY_VERSION,
                                true,
                                status);
}

int zisCallVersionedService(const CrossMemoryServerName *serverName,
                            const ZISServicePath *path, void *parm,
                            unsigned int version,
                            ZISServiceStatus *status) {

  return zisCallServiceInternal(serverName, path, parm, version, false, status);

}

/*
  This program and the accompanying materials are
  made available under the terms of the Eclipse Public License v2.0 which accompanies
  this distribution, and is available at https://www.eclipse.org/legal/epl-v20.html
  
  SPDX-License-Identifier: EPL-2.0
  
  Copyright Contributors to the Zowe Project.
*/
