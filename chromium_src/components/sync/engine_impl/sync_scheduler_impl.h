/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_IMPL_SYNC_SCHEDULER_IMPL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_IMPL_SYNC_SCHEDULER_IMPL_H_

#include "brave/components/brave_sync/jslib_messages_fwd.h"

#define BRAVE_SYNC_SCHEDULER_IMPL_H \
 public: \
  void OnNudgeSyncCycle(brave_sync::RecordsListPtr records_list) override; \
  void OnPollSyncCycle(brave_sync::GetRecordsCallback cb, \
                       base::WaitableEvent* wevent) override; \
 private: \
  friend class SyncManagerImpl; \
  brave_sync::NudgeSyncCycleDelegate nudge_sync_cycle_delegate_function_; \
  brave_sync::PollSyncCycleDelegate poll_sync_cycle_delegate_function_;

#include "../../../../../components/sync/engine_impl/sync_scheduler_impl.h"
#undef BRAVE_SYNC_SCHEDULER_IMPL_H

#endif    // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_IMPL_SYNC_SCHEDULER_IMPL_H_    // NOLINT
