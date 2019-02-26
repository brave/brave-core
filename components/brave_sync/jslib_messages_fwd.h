/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_JSLIB_MESSAGES_FWD_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_JSLIB_MESSAGES_FWD_H_

#include "base/callback.h"
#include "base/synchronization/waitable_event.h"
#include "build/build_config.h"

// TODO(darkdh): forward declaration with unique_ptr on Windows
#if defined(OS_WIN)
#include "brave/components/brave_sync/jslib_messages.h"
#else
namespace brave_sync {
namespace jslib {
class SyncRecord;
}
}
#endif

namespace syncer {
class Syncer;
}   // namespace syncer

namespace brave_sync {

typedef std::unique_ptr<jslib::SyncRecord> SyncRecordPtr;
typedef std::vector<SyncRecordPtr> RecordsList;
typedef std::unique_ptr<RecordsList> RecordsListPtr;
typedef std::pair<SyncRecordPtr, SyncRecordPtr> SyncRecordAndExisting;
typedef std::unique_ptr<SyncRecordAndExisting> SyncRecordAndExistingPtr;
typedef std::vector<SyncRecordAndExistingPtr> SyncRecordAndExistingList;

using Uint8Array = std::vector<unsigned char>;
using GetRecordsCallback =
  base::RepeatingCallback<void(std::unique_ptr<RecordsList>)>;
using NudgeSyncCycleDelegate =
  base::Callback<void(RecordsListPtr)>;
using PollSyncCycleDelegate =
  base::Callback<void(GetRecordsCallback, base::WaitableEvent* wevent)>;

}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_JSLIB_MESSAGES_FWD_H_
