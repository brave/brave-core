/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_GLUE_SYNC_ENGINE_BACKEND_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_GLUE_SYNC_ENGINE_BACKEND_H_

// chromium_src/components/sync/engine/sync_engine.h also redefines
// DisableProtocolEventForwarding include explicitly it to avoid compilation
// 'DisableProtocolEventForwarding' macro redefined
#include "components/sync/engine/sync_engine.h"

namespace syncer {
struct SyncProtocolError;
}  // namespace syncer

#define DisableProtocolEventForwarding                              \
  PermanentlyDeleteAccount(                                         \
      base::OnceCallback<void(const SyncProtocolError&)> callback); \
  void DisableProtocolEventForwarding

#include "src/components/sync/service/glue/sync_engine_backend.h"  // IWYU pragma: export

#undef DisableProtocolEventForwarding

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_GLUE_SYNC_ENGINE_BACKEND_H_
