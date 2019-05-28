/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../components/sync/engine_impl/sync_manager_impl.cc"    // NOLINT
#include "brave/components/brave_sync/jslib_messages.h"

namespace syncer {

void SyncManagerImpl::BraveInit(InitArgs* args) {
  DCHECK(args);
  scheduler_->SetNudgeAndPollDelegate(args->nudge_sync_cycle_delegate_function,
                                      args->poll_sync_cycle_delegate_function);
}

}  // namespace syncer
