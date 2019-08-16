/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_PUBLIC_BRAVE_PROFILE_SYNC_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_PUBLIC_BRAVE_PROFILE_SYNC_SERVICE_H_

#include <utility>

#include "brave/components/brave_sync/jslib_messages_fwd.h"
#include "components/sync/driver/profile_sync_service.h"

namespace base {
class WaitableEvent;
}

namespace brave_sync {

class BraveSyncService;

class BraveProfileSyncService : public syncer::ProfileSyncService {
 public:
  explicit BraveProfileSyncService(InitParams init_params)
      : syncer::ProfileSyncService(std::move(init_params)) {}
  ~BraveProfileSyncService() override {}

  virtual bool IsBraveSyncEnabled() const = 0;
  virtual void OnNudgeSyncCycle(brave_sync::RecordsListPtr records_list) = 0;
  virtual void OnPollSyncCycle(brave_sync::GetRecordsCallback cb,
                               base::WaitableEvent* wevent) = 0;

  virtual BraveSyncService* GetSyncService() const = 0;
};

}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_PUBLIC_BRAVE_PROFILE_SYNC_SERVICE_H_
