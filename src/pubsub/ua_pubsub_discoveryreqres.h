/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 */

#ifndef UA_PUBSUB_DISCOVERYREQRES_H_
#define UA_PUBSUB_DISCOVERYREQRES_H_

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#include "ua_pubsub.h"

#ifdef UA_ENABLE_PUBSUB_DISCOVERY_REQUESTRESPONSE

UA_StatusCode
sendDiscoveryResponse(UA_Server *server, UA_Byte informationType, UA_UInt16 *writerGroupId);

#endif /* UA_ENABLE_PUBSUB_DISCOVERY_REQUESTRESPONSE */

#endif /* UA_ENABLE_PUBSUB */

_UA_END_DECLS

#endif /* UA_PUBSUB_DISCOVERYREQRES_H_ */
