/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_PROFILE_SYNC_SERVICE_EXPORTS_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_PROFILE_SYNC_SERVICE_EXPORTS_H_

#include "brave/components/brave_sync/jslib_messages_fwd.h"

namespace base {
class WaitableEvent;
}

namespace brave_sync {

class BraveProfileSyncServiceExports {
 public:
   virtual bool IsBraveSyncEnabled() const = 0;
   virtual void OnNudgeSyncCycle(brave_sync::RecordsListPtr records_list) = 0;
   virtual void OnPollSyncCycle(brave_sync::GetRecordsCallback cb,
                        base::WaitableEvent* wevent) = 0;
};

}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_PROFILE_SYNC_SERVICE_EXPORTS_H_
