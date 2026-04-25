/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/sync/engine/brave_sync_manager_impl.h"

#include <utility>

#include "components/sync/engine/sync_protocol_error.h"
#include "components/sync/engine/sync_scheduler.h"

namespace syncer {

BraveSyncManagerImpl::BraveSyncManagerImpl(
    const std::string& name,
    network::NetworkConnectionTracker* network_connection_tracker)
    : SyncManagerImpl(name, network_connection_tracker) {}

BraveSyncManagerImpl::~BraveSyncManagerImpl() = default;

void BraveSyncManagerImpl::StartSyncingNormally(base::Time last_poll_time) {
  SyncManagerImpl::StartSyncingNormally(last_poll_time);
  // Remove this hack when we have FCM invalidation integrated.
  RefreshTypes(DataTypeSet::All());
}

void BraveSyncManagerImpl::PermanentlyDeleteAccount(
    base::OnceCallback<void(const SyncProtocolError&)> callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  scheduler_->SchedulePermanentlyDeleteAccount(std::move(callback));
}

}  // namespace syncer
