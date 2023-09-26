/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_SYNC_ENGINE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_SYNC_ENGINE_H_

#include "base/functional/callback_forward.h"
#include "components/sync/engine/sync_protocol_error.h"

#define DisableProtocolEventForwarding                                \
  PermanentlyDeleteAccount(                                           \
      base::OnceCallback<void(const SyncProtocolError&)> callback) {} \
  virtual void DisableProtocolEventForwarding

#include "src/components/sync/engine/sync_engine.h"  // IWYU pragma: export

#undef DisableProtocolEventForwarding

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_SYNC_ENGINE_H_
