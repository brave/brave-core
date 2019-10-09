/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_PROFILE_SYNC_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_PROFILE_SYNC_SERVICE_IMPL_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "brave/components/brave_sync/brave_sync_service.h"
#include "brave/components/brave_sync/client/brave_sync_client.h"
#include "brave/components/brave_sync/jslib_messages_fwd.h"
#include "brave/components/brave_sync/public/brave_profile_sync_service.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/sync/driver/profile_sync_service.h"
#include "services/network/public/cpp/network_connection_tracker.h"

FORWARD_DECLARE_TEST(BraveSyncServiceTest, BookmarkAdded);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, BookmarkDeleted);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, GetSyncWords);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, GetSeed);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, OnBraveSyncPrefsChanged);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, OnDeleteDevice);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, OnDeleteDeviceWhenOneDevice);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, OnDeleteDeviceWhenSelfDeleted);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, OnResetSync);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, ClientOnGetInitData);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, OnGetInitData);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, OnSaveBookmarksBaseOrder);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, OnSyncPrefsChanged);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, OnSyncDebug);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, StartSyncNonDeviceRecords);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, OnSyncReadyNewToSync);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, OnGetExistingObjects);
FORWARD_DECLARE_TEST(BraveSyncServiceTest,
                     OnSetupSyncHaveCode_Reset_SetupAgain);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, ExponentialResend);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, GetDevicesWithFetchSyncRecords);

class BraveSyncServiceTest;

namespace brave_sync {
namespace prefs {
class Prefs;
}  // namespace prefs

class BraveProfileSyncServiceImpl
    : public BraveProfileSyncService,
      public BraveSyncService,
      public network::NetworkConnectionTracker::NetworkConnectionObserver,
      public SyncMessageHandler {
 public:
  explicit BraveProfileSyncServiceImpl(Profile* profile,
                                       InitParams init_params);

  ~BraveProfileSyncServiceImpl() override;

  // BraveSyncService implementation
  void OnSetupSyncHaveCode(const std::string& sync_words,
                           const std::string& device_name) override;
  void OnSetupSyncNewToSync(const std::string& device_name) override;
  void OnDeleteDevice(const std::string& device_id) override;
  void OnResetSync() override;
  void GetSettingsAndDevices(
      const GetSettingsAndDevicesCallback& callback) override;
  void GetSyncWords() override;
  std::string GetSeed() override;
  void OnSetSyncEnabled(const bool sync_this_device) override;
  void OnSetSyncBookmarks(const bool sync_bookmarks) override;
  void OnSetSyncBrowsingHistory(const bool sync_browsing_history) override;
  void OnSetSyncSavedSiteSettings(const bool sync_saved_site_settings) override;

  // SyncMessageHandler implementation
  void BackgroundSyncStarted(bool startup) override;
  void BackgroundSyncStopped(bool shutdown) override;
  void OnSyncDebug(const std::string& message) override;
  void OnSyncSetupError(const std::string& error) override;
  void OnGetInitData(const std::string& sync_version) override;
  void OnSaveInitData(const brave_sync::Uint8Array& seed,
                      const brave_sync::Uint8Array& device_id) override;
  void OnSyncReady() override;
  void OnGetExistingObjects(const std::string& category_name,
                            std::unique_ptr<brave_sync::RecordsList> records,
                            const base::Time& last_record_time_stamp,
                            const bool is_truncated) override;
  void OnResolvedSyncRecords(
      const std::string& category_name,
      std::unique_ptr<brave_sync::RecordsList> records) override;
  void OnDeletedSyncUser() override;
  void OnDeleteSyncSiteSettings() override;
  void OnSaveBookmarksBaseOrder(const std::string& order) override;

  // syncer::SyncService implementation
  int GetDisableReasons() const override;
  CoreAccountInfo GetAuthenticatedAccountInfo() const override;
  bool IsAuthenticatedAccountPrimary() const override;

  // NetworkConnectionTracker::NetworkConnectionObserver implementation.
  void OnConnectionChanged(network::mojom::ConnectionType type) override;

  // KeyedService implementation.  This must be called exactly
  // once (before this object is destroyed).
  void Shutdown() override;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  BraveSyncClient* GetBraveSyncClient() override;
#endif

  bool IsBraveSyncEnabled() const override;
  bool IsBraveSyncInitialized() const;
  bool IsBraveSyncConfigured() const;

  syncer::ModelTypeSet GetPreferredDataTypes() const override;

  void OnNudgeSyncCycle(brave_sync::RecordsListPtr records_list) override;
  void OnPollSyncCycle(brave_sync::GetRecordsCallback cb,
                       base::WaitableEvent* wevent) override;

  BraveSyncService* GetSyncService() const override;

 private:
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, BookmarkAdded);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, BookmarkDeleted);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, GetSyncWords);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, GetSeed);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, OnBraveSyncPrefsChanged);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, OnDeleteDevice);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, OnDeleteDeviceWhenOneDevice);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest,
                           OnDeleteDeviceWhenSelfDeleted);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, OnResetSync);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, ClientOnGetInitData);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, OnSaveBookmarksBaseOrder);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, OnGetInitData);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, OnSyncPrefsChanged);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, OnSyncDebug);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, StartSyncNonDeviceRecords);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, OnSyncReadyNewToSync);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, OnGetExistingObjects);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest,
                           OnSetupSyncHaveCode_Reset_SetupAgain);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, ExponentialResend);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest,
                           GetDevicesWithFetchSyncRecords);
  friend class ::BraveSyncServiceTest;

  void SignalWaitableEvent();
  void FetchSyncRecords(const bool bookmarks,
                        const bool history,
                        const bool preferences,
                        int max_records);
  void FetchDevices();
  void SendCreateDevice();
  void SendDeviceSyncRecord(const int action,
                            const std::string& device_name,
                            const std::string& device_id,
                            const std::string& object_id);
  void OnResolvedPreferences(const brave_sync::RecordsList& records);
  void OnBraveSyncPrefsChanged(const std::string& pref);
  void NotifySyncSetupError(const std::string& error);
  void NotifySyncStateChanged();
  void NotifyHaveSyncWords(const std::string& sync_words);

  void ResetSyncInternal();
  void ForceCompleteReset();
  bool GetResettingForTest() const { return reseting_; }

  void SetPermanentNodesOrder(const std::string& base_order);

  std::unique_ptr<jslib::SyncRecord> BookmarkNodeToSyncBookmark(
      const bookmarks::BookmarkNode* node);
  // These SyncEntityInfo is for legacy device who doesn't send meta info for
  // sync entity
  void SaveSyncEntityInfo(const jslib::SyncRecord* record);
  void LoadSyncEntityInfo(jslib::SyncRecord* record);

  void CreateResolveList(
      const std::vector<std::unique_ptr<jslib::SyncRecord>>& records,
      SyncRecordAndExistingList* records_and_existing_objects);
  std::unique_ptr<SyncRecordAndExistingList> PrepareResolvedPreferences(
      const RecordsList& records);

  void SendAndPurgePendingRecords();

  void SendSyncRecords(const std::string& category_name,
                       RecordsListPtr records);
  void ResendSyncRecords(const std::string& category_name);

  void RecordSyncStateP3A() const;

  static base::TimeDelta GetRetryExponentialWaitAmount(int retry_number);
  static std::vector<unsigned> GetExponentialWaitsForTests();
  static const std::vector<unsigned> kExponentialWaits;
  static const int kMaxSendRetries;

  std::unique_ptr<brave_sync::prefs::Prefs> brave_sync_prefs_;
  // True when is in active sync chain
  bool brave_sync_configured_ = false;

  // True if we have received SyncReady from JS lib
  bool brave_sync_initialized_ = false;

  // Prevent two sequential calls OnSetupSyncHaveCode or OnSetupSyncNewToSync
  // while being initializing
  bool brave_sync_initializing_ = false;

  bool reseting_ = false;

  std::string brave_sync_words_;

  brave_sync::GetRecordsCallback get_record_cb_;
  base::WaitableEvent* wevent_ = nullptr;

  // Registrar used to monitor the brave_profile prefs.
  PrefChangeRegistrar brave_pref_change_registrar_;

  bookmarks::BookmarkModel* model_ = nullptr;

  std::unique_ptr<BraveSyncClient> brave_sync_client_;

  base::Time chain_created_time_;
  std::vector<RecordsListPtr> pending_send_records_;

  // Used to ensure that certain operations are performed on the sequence that
  // this object was created on.
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(BraveProfileSyncServiceImpl);
};
}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_PROFILE_SYNC_SERVICE_IMPL_H_
