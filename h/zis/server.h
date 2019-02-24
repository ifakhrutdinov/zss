#ifndef ZIS_SERVER_H_
#define ZIS_SERVER_H_

#include "zowetypes.h"
#include "collections.h"
#include "crossmemory.h"
#include "zos.h"

#include "zis/parm.h"
#include "zis/service.h"

ZOWE_PRAGMA_PACK

typedef struct ZISContext_tag {
  char eyecatcher[8];
#define ZIS_CONTEXT_EYECATCHER "ZISCNTXT"
  struct STCBase_tag *stcBase;
  ZISParmSet *parms;
  CrossMemoryServer *cmServer;
  CrossMemoryServerGlobalArea *cmsGA;
  struct ZISServerAnchor_tag *zisAnchor;
  struct ZISPlugin_tag *firstPlugin;
} ZISContext;

typedef struct ZISServerAnchor_tag {
  char eyecatcher[8];
#define ZIS_SERVER_ANCHOR_EYECATCHER "ZISSRVAN"
  int version;
#define ZIS_SERVER_ANCHOR_VERSION 1
  int key : 8;
  unsigned int subpool : 8;
  unsigned short size;
  int flags;

  char reservred0[4];

  PAD_LONG(0, struct CrossMemoryMap_tag *serviceTable);

  PAD_LONG(1, struct ZISPluginAnchor_tag *firstPlugin);

  char reservred2[24];

} ZISServerAnchor;

typedef __packed struct ZISServiceRouterData_tag {
  char eyecatcher[8];
#define ZIS_SERVICE_ROUTER_EYECATCHER "ZISSREYE"
  ZISServicePath targetServicePath;
  PAD_LONG(0x00, void *targetServiceParm);
} ZISServiceRouterParm;

#define ZIS_SERVICE_ID_SRVC_ROUTER_SS 64
#define ZIS_SERVICE_ID_SRVC_ROUTER_CP 65

ZOWE_PRAGMA_PACK_RESET

#define RC_ZIS_OK       0
#define RC_ZIS_ERROR    8

#endif /* ZIS_SERVER_H_ */
