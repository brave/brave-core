/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SYNC_CONTROLLER_IMPL_H_
#define BRAVE_COMPONENTS_SYNC_CONTROLLER_IMPL_H_

#include <map>
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "brave/components/brave_sync/controller.h"
#include "brave/components/brave_sync/cansendbookmarks.h"
#include "brave/components/brave_sync/client/client.h"
#include "brave/components/brave_sync/can_send_history.h"

class Browser;
class SyncUI;

namespace base {
  class RepeatingTimer;
  class SequencedTaskRunner;
}

namespace extensions {
class BraveSyncEventRouter;
}

namespace bookmarks {
  class BookmarkNode;
}

namespace history {
  class URLResult;
}

namespace brave_sync {

namespace storage {
  class ObjectMap;
}

namespace prefs {
  class Prefs;
}

class SyncDevices;
class Controller;
class Settings;
class Bookmarks;
class History;

class ControllerImpl : public Controller,
                       public SyncLibToBrowserHandler,
                       public ControllerForBookmarksExports,
                       public CanSendSyncHistory {
public:
  ControllerImpl(Profile *profile);
  ~ControllerImpl() override;

  // KeyedService overrides
  void Shutdown() override;

  // Controller messages from UI
  void OnSetupSyncHaveCode(const std::string &sync_words,
    const std::string &device_name) override;
  void OnSetupSyncNewToSync(const std::string &device_name) override;
  void OnDeleteDevice(const std::string &device_id) override;
  void OnResetSync() override;
  void GetSyncWords() override;
  std::string GetSeed() override;
  void SetupUi(SyncUI *sync_ui) override;

  void GetSettingsAndDevices(const GetSettingsAndDevicesCallback &callback) override;
  void GetSettingsAndDevicesImpl(std::unique_ptr<brave_sync::Settings> settings, const GetSettingsAndDevicesCallback &callback);

private:
  DISALLOW_COPY_AND_ASSIGN(ControllerImpl);
  friend struct base::DefaultSingletonTraits<ControllerImpl>;

  void SetProfile(Profile *profile);

  void InitJsLib(const bool &setup_new_sync);

  // Compiler complains at
  void OnMessageFromSyncReceived() override;

  // SyncLibToBrowserHandler overrides
  void OnSyncDebug(const std::string &message) override;
  void OnSyncSetupError(const std::string &error) override;
  void OnGetInitData(const std::string &sync_version) override;
  void OnSaveInitData(const Uint8Array &seed, const Uint8Array &device_id) override;
  void OnSyncReady() override;
  void OnGetExistingObjects(const std::string &category_name,
    std::unique_ptr<RecordsList> records,
    const base::Time &last_record_time_stamp,
    const bool &is_truncated) override;
  void OnResolvedSyncRecords(const std::string &category_name,
    std::unique_ptr<RecordsList> records) override;
  void OnDeletedSyncUser() override;
  void OnDeleteSyncSiteSettings() override;
  void OnSaveBookmarksBaseOrder(const std::string &order) override;
  void OnSaveBookmarkOrder(const std::string &order,
    const std::string &prev_order, const std::string &next_order) override;
  void OnSyncWordsPrepared(const std::string &words) override;
  void OnBytesFromSyncWordsPrepared(const Uint8Array &bytes, const std::string &error_message) override;

  void OnResolvedPreferences(const RecordsList &records);
  void OnResolvedBookmarks(const RecordsList &records);
  void OnResolvedHistorySites(const RecordsList &records);

  // Runs in task runner to perform file work
  void OnGetExistingObjectsFileWork(const std::string &category_name,
     std::unique_ptr<RecordsList> records,
     const base::Time &last_record_time_stamp,
     const bool &is_truncated
  );
  void OnResolvedSyncRecordsFileWork(const std::string &category_name,
    std::unique_ptr<RecordsList> records);

  void CreateUpdateDeleteBookmarksFileWork(
    const int &action,
    const std::vector<const bookmarks::BookmarkNode*> &list,
    const std::map<const bookmarks::BookmarkNode*, std::string> &order_map,
    const bool &addIdsToNotSynced,
    const bool &isInitialSync);

  void ShutdownFileWork();

  void OnDeleteDeviceFileWork(const std::string &device_id);
  void OnResetSyncFileWork(const std::string &device_id);
  void OnResetSyncPostFileUiWork();

  void OnSaveBookmarkOrderFileWork(const int64_t &bookmark_local_id, const std::string &order);

  // Other private methods
  void RequestSyncData();
  void FetchSyncRecords(const bool &bookmarks, const bool &history,
    const bool &preferences, int64_t start_at, int max_records);

  SyncRecordPtr PrepareResolvedDevice(const std::string &object_id);

  SyncRecordAndExistingList PrepareResolvedResponse(
    const std::string &category_name,
    const RecordsList &records);
  void SendResolveSyncRecords(const std::string &category_name,
    const SyncRecordAndExistingList& records_and_existing_objects);

  void SendCreateDevice();

  void SendAllLocalBookmarks();
  void SendAllLocalHistorySites();

  // CanSendBookMarks overrides
  void CreateUpdateDeleteBookmarks(
    const int &action,
    const std::vector<const bookmarks::BookmarkNode*> &list,
    const std::map<const bookmarks::BookmarkNode*, std::string> &order_map,
    const bool &addIdsToNotSynced,
    const bool &isInitialSync) override;

  void BookmarkMoved(
    const int64_t &node_id,
    const int64_t &prev_item_id,
    const int64_t &next_item_id) override;

  void BookmarkMovedQueryNewOrderUiWork(
    const int64_t &node_id,
    const std::string &prev_item_order,
    const std::string &next_item_order);

  void CreateUpdateDeleteHistorySites(
    const int &action,
    //history::QueryResults::URLResultVector list,
    const std::vector<history::URLResult> &list,
    const bool &addIdsToNotSynced,
    const bool &isInitialSync);

  // CanSendHistory overrides
  void HaveInitialHistory(history::QueryResults* results) override;

  void SendDeviceSyncRecord(const int &action,
    const std::string &device_name,
    const std::string &device_id,
    const std::string &object_id);

  enum NotSyncedRecordsOperation {
    GetItems,
    AddItems,
    DeleteItems
  };

  void SetUpdateDeleteDeviceName_Ext(
    const std::string &action,
    const std::string &deviceName,
    const std::string &deviceId,
    const std::string &objectId);

  std::vector<std::string> SaveGetDeleteNotSyncedRecords(
    const std::string &recordType, const std::string &action,
    const std::vector<std::string> &ids,
    NotSyncedRecordsOperation operation);

  std::string GenerateObjectIdWithMapCheck(const std::string &local_id);

  void StartLoop();
  void StopLoop();
  void LoopProc();
  void LoopProcThreadAligned();

  void GetExistingHistoryObjects(
    const RecordsList &records,
    const base::Time &last_record_time_stamp,
    const bool &is_truncated );

  base::SequencedTaskRunner *GetTaskRunner() override;

  void PushRRContext(const std::string &prev_order, const std::string &next_order, const int64_t &context);
  int64_t PopRRContext(const std::string &prev_order, const std::string &next_order);

  // Messages Controller => SyncWebUi
  SyncUI *sync_ui_;

  BraveSyncClient *sync_client_;

  // True if we have received SyncReady from JS lib
  bool sync_initialized_;
  // Version received from "get-init-data" command
  std::string sync_version_;

  // Mark members with an small life time
  class TempStorage {
  public:
    TempStorage();
    ~TempStorage();
    // This should be used only for passing
    // between OnSetupSyncHaveCode or OnSetupSyncNewToSync to OnSaveInitData
    std::string device_name_;
    // Between OnWordsToBytesDone => InitJsLib|OnSaveInitData
    std::string seed_str_;
  };
  TempStorage temp_storage_;

  std::auto_ptr<brave_sync::prefs::Prefs> sync_prefs_;
  std::unique_ptr<brave_sync::Settings> settings_;
  std::unique_ptr<brave_sync::storage::ObjectMap> sync_obj_map_;
  std::unique_ptr<brave_sync::Bookmarks> bookmarks_;
  std::unique_ptr<brave_sync::History> history_;

  Profile *profile_;

  bool seen_get_init_data_ = false;

  std::map<std::tuple<std::string, std::string>, int64_t> rr_map_; // Map to keep tracking between request and response on query bookmarks order

  std::unique_ptr<base::RepeatingTimer> timer_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  SEQUENCE_CHECKER(sequence_checker_);
};

} // namespace brave_sync

#endif //BRAVE_COMPONENTS_SYNC_CONTROLLER_IMPL_H_
