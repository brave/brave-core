/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/sync/engine_impl/sync_scheduler_impl.h"

#include "brave/components/brave_sync/jslib_messages.h"

namespace syncer {

void SyncSchedulerImpl::OnNudgeSyncCycle(
    brave_sync::RecordsListPtr records_list) {
  DCHECK(nudge_sync_cycle_delegate_function_);
  nudge_sync_cycle_delegate_function_.Run(std::move(records_list));
}

void SyncSchedulerImpl::OnPollSyncCycle(brave_sync::GetRecordsCallback cb,
                                        base::WaitableEvent* wevent) {
  DCHECK(poll_sync_cycle_delegate_function_);
  poll_sync_cycle_delegate_function_.Run(cb, wevent);
}

}  // namespace syncer

#define BRAVE_SYNC_SCHEDULER_IMPL_TRY_SYNC_CYCLE_JOB \
  SyncCycle cycle(cycle_context_, this); \
  if (mode_ != CONFIGURATION_MODE) { \
    syncer_->DownloadBraveRecords(&cycle); \
  }

#include "../../../../../components/sync/engine_impl/sync_scheduler_impl.cc"  // NOLINT
