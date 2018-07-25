/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef H_BRAVE_COMPONENTS_BRAVE_SYNC_HISTORY_H
#define H_BRAVE_COMPONENTS_BRAVE_SYNC_HISTORY_H

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "base/task/cancelable_task_tracker.h"
#include "components/history/core/browser/history_service_observer.h"



class Profile;

// namespace history {
// class HistoryService;
// }

namespace brave_sync {

namespace storage {
  class ObjectMap;
}

namespace jslib {
  class SyncRecord;
  class Site;
}
// TODO, AB: move to fwd.h
typedef std::unique_ptr<jslib::SyncRecord> SyncRecordPtr;
typedef std::vector<SyncRecordPtr> RecordsList;

class CanSendSyncHistory;

class History : public history::HistoryServiceObserver {
public:
  History(Profile* profile,
          /*history::HistoryService* history_service*/
          CanSendSyncHistory *send_history);
  ~History() override;

  void SetObjectMap(storage::ObjectMap* sync_obj_map);
  void SetThisDeviceId(const std::string &device_id);


  void GetAllHistory();

  std::unique_ptr<RecordsList> NativeHistoryToSyncRecords(
    const history::QueryResults::URLResultVector &list,
    int action);

  std::unique_ptr<jslib::SyncRecord> GetResolvedHistoryValue(const std::string &object_id);

private:

  // history::HistoryServiceObserver.
  void OnURLVisited(history::HistoryService* history_service,
                    ui::PageTransition transition,
                    const history::URLRow& row,
                    const history::RedirectList& redirects,
                    base::Time visit_time) override;
  void OnURLsDeleted(history::HistoryService* history_service,
                     const history::DeletionInfo& deletion_info) override;

  void GetAllHistoryComplete(history::QueryResults* results);

  std::string GetOrCreateObjectByLocalId(const int64_t &local_id);

  std::unique_ptr<jslib::Site> GetFromUrlRow(const history::URLRow* url_row);


  Profile* profile_;
  ScopedObserver<history::HistoryService, history::HistoryServiceObserver>
      history_service_observer_;

  base::CancelableTaskTracker task_tracker_;

  CanSendSyncHistory *send_history_;

  storage::ObjectMap* sync_obj_map_;
  std::string device_id_;

  DISALLOW_COPY_AND_ASSIGN(History);
};

} // namespace brave_sync


#endif // H_BRAVE_COMPONENTS_BRAVE_SYNC_HISTORY_H
