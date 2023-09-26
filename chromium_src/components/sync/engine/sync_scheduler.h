/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_SYNC_SCHEDULER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_SYNC_SCHEDULER_H_

#include "components/sync/engine/sync_protocol_error.h"

#define OnCredentialsUpdated                                          \
  SchedulePermanentlyDeleteAccount(                                   \
      base::OnceCallback<void(const SyncProtocolError&)> callback) {} \
  virtual void OnCredentialsUpdated

#include "src/components/sync/engine/sync_scheduler.h"  // IWYU pragma: export

#undef OnCredentialsUpdated

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_SYNC_SCHEDULER_H_
