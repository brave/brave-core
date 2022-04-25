/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_SYNCER_PROTO_UTIL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_SYNCER_PROTO_UTIL_H_

#define SetProtocolVersion                                                \
  SaveServerErrorMessage(const sync_pb::ClientToServerResponse& response, \
                         StatusController* status_controller);            \
  static void SetProtocolVersion

#include "src/components/sync/engine/syncer_proto_util.h"

#undef SetProtocolVersion

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_SYNCER_PROTO_UTIL_H_
