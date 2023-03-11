/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_SYNC_MANAGER_IMPL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_SYNC_MANAGER_IMPL_H_

// chromium_src/components/sync/engine/sync_manager.h also redefines
// ShutdownOnSyncThread. Include it explicitly to avoid compilation error
// 'ShutdownOnSyncThread' macro redefined
#include "components/sync/engine/sync_manager.h"

#define ShutdownOnSyncThread         \
  Unused() {}                        \
  friend class BraveSyncManagerImpl; \
  void ShutdownOnSyncThread

#include "src/components/sync/engine/sync_manager_impl.h"  // IWYU pragma: export

#undef ShutdownOnSyncThread

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_SYNC_MANAGER_IMPL_H_
