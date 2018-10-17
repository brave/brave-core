/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SYNC_BRAVE_SYNC_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_SYNC_BRAVE_SYNC_SERVICE_IMPL_H_

#include <map>

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "base/time/time.h"
#include "brave/components/brave_sync/brave_sync_service.h"
#include "brave/components/brave_sync/client/brave_sync_client.h"

namespace base {
class RepeatingTimer;
}

namespace brave_sync {

class SyncDevices;
class Settings;
class BookmarkChangeProcessor;

namespace prefs {
class Prefs;
}  // namespace prefs

using SendDeviceSyncRecordCallback = base::OnceCallback<void(const int,
                                                           const std::string&,
                                                           const std::string&,
                                                           const std::string&)>;

class BraveSyncServiceImpl : public BraveSyncService,
                             public SyncMessageHandler {
 public:
  BraveSyncServiceImpl(Profile *profile);
  ~BraveSyncServiceImpl() override;

  // KeyedService overrides
  void Shutdown() override;

  // BraveSyncService messages from UI
  void OnSetupSyncHaveCode(const std::string& sync_words,
    const std::string& device_name) override;
  void OnSetupSyncNewToSync(const std::string& device_name) override;
  void OnDeleteDevice(const std::string& device_id) override;
  void OnResetSync() override;
  void GetSyncWords() override;
  std::string GetSeed() override;
  void OnSetSyncEnabled(const bool sync_this_device) override;
  void OnSetSyncBookmarks(const bool sync_bookmarks) override;
  void OnSetSyncBrowsingHistory(const bool sync_browsing_history) override;
  void OnSetSyncSavedSiteSettings(const bool sync_saved_site_settings) override;

  void GetSettingsAndDevices(
      const GetSettingsAndDevicesCallback& callback) override;

  bool IsSyncConfigured();
  bool IsSyncInitialized();

  BraveSyncClient* GetSyncClient() override;

 private:
  // SyncMessageHandler overrides
  void BackgroundSyncStarted() override;
  void BackgroundSyncStopped() override;
  void OnSyncDebug(const std::string& message) override;
  void OnSyncSetupError(const std::string& error) override;
  void OnGetInitData(const std::string& sync_version) override;
  void OnSaveInitData(const Uint8Array& seed,
                      const Uint8Array& device_id) override;
  void OnSyncReady() override;
  void OnGetExistingObjects(const std::string& category_name,
    std::unique_ptr<RecordsList> records,
    const base::Time &last_record_time_stamp,
    const bool is_truncated) override;
  void OnResolvedSyncRecords(const std::string& category_name,
    std::unique_ptr<RecordsList> records) override;
  void OnDeletedSyncUser() override;
  void OnDeleteSyncSiteSettings() override;
  void OnSaveBookmarksBaseOrder(const std::string& order) override;
  void OnSaveBookmarkOrder(const std::string& order,
                           const std::string& prev_order,
                           const std::string& next_order,
                           const std::string& parent_order) override;
  void OnSyncWordsPrepared(const std::string& words) override;

  void OnResolvedHistorySites(const RecordsList &records);
  void OnResolvedPreferences(const RecordsList &records);
  std::unique_ptr<SyncRecordAndExistingList> PrepareResolvedPreferences(
    const RecordsList& records);

  // Other private methods
  void RequestSyncData();
  void FetchSyncRecords(const bool bookmarks, const bool history,
    const bool preferences, int max_records);

  void SendCreateDevice();
  void SendDeviceSyncRecord(
      const int action,
      const std::string& device_name,
      const std::string& device_id,
      const std::string& object_id);

  void SetUpdateDeleteDeviceName_Ext(
    const std::string& action,
    const std::string& deviceName,
    const std::string& deviceId,
    const std::string& objectId);

  void StartLoop();
  void StopLoop();
  void LoopProc();
  void LoopProcThreadAligned();

  void GetExistingHistoryObjects(
    const RecordsList &records,
    const base::Time &last_record_time_stamp,
    const bool is_truncated );

  void NotifyLogMessage(const std::string& message);
  void NotifySyncStateChanged();
  void NotifyHaveSyncWords(const std::string& sync_words);

  std::unique_ptr<BraveSyncClient> sync_client_;

  // True when is in active sync chain
  bool sync_configured_ = false;
  // True if we have received SyncReady from JS lib
  bool sync_initialized_ = false;

  // Prevent two sequential calls OnSetupSyncHaveCode or OnSetupSyncNewToSync
  // while being initializing
  bool initializing_ = false;

  std::string sync_words_;

  std::unique_ptr<brave_sync::Settings> settings_;

  Profile *profile_;
  std::unique_ptr<brave_sync::prefs::Prefs> sync_prefs_;

  std::unique_ptr<BookmarkChangeProcessor> bookmark_change_processor_;
  // Moment when FETCH_SYNC_RECORDS was sent,
  // will be saved on GET_EXISTING_OBJECTS to be sure request was processed
  base::Time last_time_fetch_sent_;

  std::unique_ptr<base::RepeatingTimer> timer_;

  // send unsynced records in batches
  base::TimeDelta unsynced_send_interval_;
  uint64_t initial_sync_records_remaining_;

  DISALLOW_COPY_AND_ASSIGN(BraveSyncServiceImpl);
};

} // namespace brave_sync

#endif //BRAVE_COMPONENTS_SYNC_BRAVE_SYNC_SERVICE_IMPL_H_
