/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SYNC_CONTROLLER_IMPL_H_
#define BRAVE_COMPONENTS_SYNC_CONTROLLER_IMPL_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "brave/browser/ui/webui/sync/sync_js_layer.h"
#include "brave/components/brave_sync/controller.h"
#include "brave/components/brave_sync/cansendbookmarks.h"
#include "brave/components/brave_sync/client/client.h"

class Browser;
class SyncJsLayer;
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

namespace brave_sync {

namespace storage {
  class BraveSyncObjMap;
}

namespace prefs {
  class BraveSyncPrefs;
}

class SyncDevices;
class BraveSyncController;
struct BraveSyncSettings;
class BraveSyncBookmarks;

class BraveSyncControllerImpl : public BraveSyncController,
                            ///public SyncJsLayerResponseReceiver,///
                            public SyncLibToBrowserHandler,
                            public BrowserListObserver,
                            public CanSendSyncBookmarks {
public:
  BraveSyncControllerImpl();
  ~BraveSyncControllerImpl() override;

  static BraveSyncControllerImpl* GetInstance();

  // BrowserListObserver overrides:
  void OnBrowserAdded(Browser* browser) override;
  void OnBrowserSetLastActive(Browser* browser) override;

  // BraveSyncController messages from UI
  void OnSetupSyncHaveCode(const std::string &sync_words,
    const std::string &device_name) override;
  void OnSetupSyncNewToSync(const std::string &device_name) override;
  void OnDeleteDevice(const std::string &device_id) override;
  void OnResetSync() override;
  void GetSettings(BraveSyncSettings &settings) override;
  void GetDevices(SyncDevices &devices) override;
  void GetSyncWords() override;
  std::string GetSeed() override;

  void SetupJsLayer(SyncJsLayer *sync_js_layer) override;
  void SetupUi(SyncUI *sync_ui) override;

private:
  DISALLOW_COPY_AND_ASSIGN(BraveSyncControllerImpl);
  friend struct base::DefaultSingletonTraits<BraveSyncControllerImpl>;

  void LoadJsLibPseudoTab();

  void InitJsLib(const bool &setup_new_sync);
  void CallJsLibBV(const base::Value &command,
    const base::Value &arg1, const base::Value &arg2, const base::Value &arg3,
    const base::Value &arg4);
  void CallJsLibStr(const std::string &command,
    const std::string &arg1, const std::string &arg2, const std::string &arg3,
    const std::string &arg4);

  // SyncJsLayerResponseReceiver methods and derived methods
  //void OnJsLibMessage(const std::string &message, const base::ListValue* args) override; //SyncJsLayerResponseReceiver::OnJsLibMessage
  //


  // Compiler complains at
  void OnMessageFromSyncReceived() override;

  // SyncLibToBrowserHandler overrides
  void OnSyncDebug(const std::string &message) override;
  void OnSyncSetupError(const std::string &error) override;
  void OnGetInitData(const std::string &sync_version) override;
  void OnSaveInitData(const Uint8Array &seed, const Uint8Array &device_id) override;
  void OnSyncReady() override;
  void OnGetExistingObjects(const std::string &category_name,
    const RecordsList &records, const base::Time &last_record_time_stamp) override;
  void OnResolvedSyncRecords(const std::string &category_name,
    const RecordsList &records) override;
  void OnDeletedSyncUser() override;
  void OnDeleteSyncSiteSettings() override;
  void OnSaveBookmarksBaseOrder(const std::string &order) override;
  void OnSaveBookmarkOrder(const std::string &order,
    const std::string &prev_order, const std::string &next_order) override;
  // Temporary from SyncJsLayerResponseReceiver
  void OnJsLibMessage(const std::string &message, const base::ListValue* args) override;


  void OnGotInitData_(const base::ListValue* args);
  void OnWordsToBytesDone_(const base::ListValue* args);
  void OnBytesToWordsDone_(const base::ListValue* args);
  void OnSyncReady_(const base::ListValue* args);
  void OnSaveInitData_(const base::ListValue* args);
  void OnGetExistingObjects_(const base::ListValue* args);
  void OnResolvedSyncRecords_(const base::ListValue* args);
  void OnSyncDebug_(const base::ListValue* args);

  void OnResolvedPreferences(const std::string &category_name,
    std::unique_ptr<base::Value> records_v);
  void OnResolvedBookmarks(std::unique_ptr<base::Value> records_v);
  void OnResolvedHistorySites(const std::string &category_name,
    std::unique_ptr<base::Value> records_v);

  void RequestSyncData();
  void FetchSyncRecords(const bool &bookmarks,
    const bool &history, const bool &preferences, int64_t start_at, int max_records);

  std::unique_ptr<base::Value> PrepareResolvedResponse(const std::string &category_name, const std::unique_ptr<base::Value> &records);
  std::unique_ptr<base::Value> PrepareResolvedDevice(const std::string &object_id);
  void SendResolveSyncRecords(const std::string &category_name, const base::Value* response);

  void SendCreateDevice();
  void SendAllLocalBookmarks();
  void SendAllLocalHistorySites();

  void CreateUpdateDeleteBookmarks(
    const int &action,
    const std::vector<const bookmarks::BookmarkNode*> &list,
    const bool &addIdsToNotSynced,
    const bool &isInitialSync) override;

  std::string CreateDeviceCreationRecord(
    const std::string &deviceName,
    const std::string &objectId,
    const std::string &action,
    const std::string &deviceId);

  void SendSyncRecords(const std::string &recordType,
    const std::string &recordsJSON,
    const std::string &action,
    const std::vector<std::string> &ids  );

  enum NotSyncedRecordsOperation {
    GetItems,
    AddItems,
    DeleteItems
  };

  void SetUpdateDeleteDeviceName(
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

  // Messages Controller => SyncWebUi
  SyncUI *sync_ui_;

  // Messages Controller => JS lib
  //SyncJsLayer *sync_js_layer_;//
  BraveSyncClient *sync_client_;

  // True if we have received SyncReady from JS lib
  bool sync_initialized_;

  // Mark members with an small life time
  class TempStorage {
  public:
    TempStorage();
    ~TempStorage();
    // This should be used only for passing
    // between OnSetupSyncHaveCode or OnSetupSyncNewToSync to OnSaveInitData
    std::string device_name_;
    // Between OnWordsToBytesDone => InitJsLib|OnGotInitData
    std::vector<char> seed_;
    // Between OnWordsToBytesDone => OnSaveInitData
    std::string seed_str_;
  };
  TempStorage temp_storage_;

  std::auto_ptr<brave_sync::prefs::BraveSyncPrefs> sync_prefs_;
  std::unique_ptr<BraveSyncSettings> settings_;
  std::unique_ptr<storage::BraveSyncObjMap> sync_obj_map_;
  std::unique_ptr<BraveSyncBookmarks> bookmarks_;
  std::unique_ptr<extensions::BraveSyncEventRouter> brave_sync_event_router_;

  Browser *browser_;

  std::unique_ptr<base::RepeatingTimer> timer_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  SEQUENCE_CHECKER(sequence_checker_);
};

} // namespace brave_sync

#endif //BRAVE_COMPONENTS_SYNC_CONTROLLER_IMPL_H_
