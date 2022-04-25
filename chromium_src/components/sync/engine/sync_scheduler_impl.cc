/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_HANDLE_CONFIGURATION_FAILURE \
  HandleBraveConfigurationFailure(cycle.status_controller());

#include "src/components/sync/engine/sync_scheduler_impl.cc"

#undef BRAVE_HANDLE_CONFIGURATION_FAILURE

namespace syncer {

namespace {

const char kNigoriFolderNotReadyError[] =
    "nigori root folder entity is not ready yet";

}  // namespace

void SyncSchedulerImpl::HandleBraveConfigurationFailure(
    const StatusController& status_controller) {
  if (status_controller.get_last_server_error_message() ==
      kNigoriFolderNotReadyError) {
    VLOG(1) << "Got nigori root folder error from sync server. Override wait "
               "interval to 3 sec";
    wait_interval_ = std::make_unique<WaitInterval>(
        WaitInterval::BlockingMode::kThrottled, base::Seconds(3));
  }
}

}  // namespace syncer
