

/*
  This program and the accompanying materials are
  made available under the terms of the Eclipse Public License v2.0 which accompanies
  this distribution, and is available at https://www.eclipse.org/legal/epl-v20.html

  SPDX-License-Identifier: EPL-2.0

  Copyright Contributors to the Zowe Project.
*/

#include <metal/metal.h>
#include <metal/stddef.h>

#include "zowetypes.h"
#include "crossmemory.h"
#include "zis/plugin.h"
#include "zis/service.h"

#include "services/echoservice.h"

ZISPlugin *getPluginDescriptor() {

  ZISPluginName pluginName = {.text = "ECHO            "};
  ZISPlugin *plugin = zisCreatePlugin(pluginName, 1, ZIS_PLUGIN_FLAG_LPA);
  if (plugin == NULL) {
    return NULL;
  }

  ZISServiceName serviceName = {.text = "ECHO-MESSAGE    "};
  ZISService service = zisCreateSpaceSwitchService(serviceName, NULL, NULL,
                                                   serveEchoedMessage);

  zisPluginAddService(plugin, service);

  return plugin;
}


/*
  This program and the accompanying materials are
  made available under the terms of the Eclipse Public License v2.0 which accompanies
  this distribution, and is available at https://www.eclipse.org/legal/epl-v20.html

  SPDX-License-Identifier: EPL-2.0

  Copyright Contributors to the Zowe Project.
*/

