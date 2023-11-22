/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_GLUE_SYNC_ENGINE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_GLUE_SYNC_ENGINE_IMPL_H_

#include "base/functional/callback_forward.h"
// chromium_src/components/sync/engine/sync_engine.h also redefines
// DisableProtocolEventForwarding include explicitly it to avoid compilation
// error 'DisableProtocolEventForwarding' macro redefined
#include "components/sync/engine/sync_engine.h"
#include "components/sync/engine/sync_protocol_error.h"

#define DisableProtocolEventForwarding                                       \
  PermanentlyDeleteAccount(                                                  \
      base::OnceCallback<void(const SyncProtocolError&)> callback) override; \
  void DisableProtocolEventForwarding

#include "src/components/sync/service/glue/sync_engine_impl.h"  // IWYU pragma: export

#undef DisableProtocolEventForwarding

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_GLUE_SYNC_ENGINE_IMPL_H_
