/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_SYNC_MANAGER_IMPL_H_
#define BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_SYNC_MANAGER_IMPL_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "components/sync/engine/sync_manager_impl.h"
#include "components/sync/protocol/sync_protocol_error.h"

namespace syncer {

class BraveSyncManagerImpl : public SyncManagerImpl {
 public:
  BraveSyncManagerImpl(
      const std::string& name,
      network::NetworkConnectionTracker* network_connection_tracker);
  BraveSyncManagerImpl(const BraveSyncManagerImpl&) = delete;
  BraveSyncManagerImpl& operator=(const BraveSyncManagerImpl&) = delete;
  ~BraveSyncManagerImpl() override;

  void StartSyncingNormally(base::Time last_poll_time) override;

  void PermanentlyDeleteAccount(
      base::OnceCallback<void(const SyncProtocolError&)> callback) override;
};

}  // namespace syncer

#endif  // BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_SYNC_MANAGER_IMPL_H_
