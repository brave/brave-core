/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_SYNC_SCHEDULER_IMPL_HANDLE_FAILURE \
  HandleBraveConfigurationFailure(model_neutral_state);

#include "src/components/sync/engine/sync_scheduler_impl.cc"
#include "base/callback_forward.h"

#undef BRAVE_SYNC_SCHEDULER_IMPL_HANDLE_FAILURE

#include "brave/components/sync/engine/brave_sync_server_commands.h"

namespace syncer {

const char kNigoriFolderNotReadyError[] =
    "nigori root folder entity is not ready yet";

void SyncSchedulerImpl::HandleBraveConfigurationFailure(
    const ModelNeutralState& model_neutral_state) {
  if (model_neutral_state.last_server_error_message ==
      kNigoriFolderNotReadyError) {
    VLOG(1) << "Got nigori root folder error from sync server. Override wait "
               "interval to 3 sec";
    wait_interval_ = std::make_unique<WaitInterval>(
        WaitInterval::BlockingMode::kThrottled, base::Seconds(3));
  }
}

void SyncSchedulerImpl::SchedulePermanentlyDeleteAccount(
    base::OnceClosure callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&SyncSchedulerImpl::PermanentlyDeleteAccountImpl,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void SyncSchedulerImpl::PermanentlyDeleteAccountImpl(
    base::OnceClosure callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  SyncCycle cycle(cycle_context_, this);
  BraveSyncServerCommands::PermanentlyDeleteAccount(&cycle,
                                                    std::move(callback));
}

}  // namespace syncer
