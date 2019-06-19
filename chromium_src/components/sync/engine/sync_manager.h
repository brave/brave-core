/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_SYNC_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_SYNC_MANAGER_H_

#include "brave/components/brave_sync/jslib_messages_fwd.h"

namespace syncer {
class Syncer;
}   // namespace syncer

#define BRAVE_SYNC_MANAGER_INIT_ARGS_H \
brave_sync::NudgeSyncCycleDelegate nudge_sync_cycle_delegate_function; \
brave_sync::PollSyncCycleDelegate poll_sync_cycle_delegate_function;

#include "../../../../../components/sync/engine/sync_manager.h"
#undef BRAVE_SYNC_MANAGER_INIT_ARGS_H

#endif    // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_SYNC_MANAGER_H_
