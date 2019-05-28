/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../../components/sync/driver/glue/sync_engine_impl.cc"    // NOLINT

#include "brave/chromium_src/components/sync/driver/glue/sync_engine_impl.h"

#include <memory>
#include <utility>

#include "brave/components/brave_sync/jslib_messages.h"

namespace syncer {

void SyncEngineImpl::HandleNudgeSyncCycle(
    brave_sync::RecordsListPtr records_list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(nudge_sync_cycle_delegate_function_);
  nudge_sync_cycle_delegate_function_.Run(std::move(records_list));
}

void SyncEngineImpl::HandlePollSyncCycle(
    GetRecordsCallback cb,
    base::WaitableEvent* wevent) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(poll_sync_cycle_delegate_function_);
  poll_sync_cycle_delegate_function_.Run(cb, wevent);
}

void SyncEngineImpl::DispatchGetRecordsCallback(
    GetRecordsCallback cb, std::unique_ptr<RecordsList> records) {
  sync_task_runner_->PostTask(
    FROM_HERE,
    base::BindOnce(&SyncEngineBackend::DoDispatchGetRecordsCallback, backend_,
                   cb, std::move(records)));
}

}   // namespace syncer
