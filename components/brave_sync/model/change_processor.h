/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_MODEL_CHANGE_PROCESSOR_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_MODEL_CHANGE_PROCESSOR_H_

#include "base/macros.h"
#include "base/time/time.h"
#include "brave/components/brave_sync/client/client_ext_impl_data.h"

namespace brave_sync {

class ChangeProcessor {
 public:
  virtual ~ChangeProcessor() = default;

  // start processing local changes
  virtual void Start() = 0;
  // stop processing local changes
  virtual void Stop() = 0;

  // reset all sync data, but do not delete local records
  virtual void Reset() = 0;

  // send all syncable data
  virtual uint64_t InitialSync() = 0;

  // get all local sync data matching `records` and return the matched pair
  // in `records_and_existing_objects`
  virtual void GetAllSyncData(
      const std::vector<std::unique_ptr<jslib::SyncRecord>>& records,
      SyncRecordAndExistingList* records_and_existing_objects) = 0;
  // update local data from `records`
  virtual void ApplyChangesFromSyncModel(const RecordsList& records) = 0;
  // send any new records that have not yet been synced to the server
  // for each record, if the last synced time is less than
  // `unsynced_send_interval` then skip sending it this time
  virtual void SendUnsynced(base::TimeDelta unsynced_send_interval) = 0;
};

}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_MODEL_CHANGE_PROCESSOR_H_
