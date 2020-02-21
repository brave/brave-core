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
#include "components/bookmarks/browser/bookmark_model_observer.h"
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
FORWARD_DECLARE_TEST(BraveSyncServiceTest, OnResetSyncWhenOffline);
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
FORWARD_DECLARE_TEST(BraveSyncServiceTest, SendCompact);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, SetSyncEnabled);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, SetSyncDisabled);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, IsSyncReadyOnNewProfile);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, SetThisDeviceCreatedTime);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, InitialFetchesStartWithZero);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, DeviceIdV2Migration);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, DeviceIdV2MigrationDupDeviceId);
FORWARD_DECLARE_TEST(BraveSyncServiceTestDelayedLoadModel,
                     OnSyncReadyModelNotYetLoaded);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, IsOtherBookmarksFolder);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, ProcessOtherBookmarksFolder);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, ProcessOtherBookmarksChildren);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, CheckOtherBookmarkRecord);
FORWARD_DECLARE_TEST(BraveSyncServiceTest, CheckOtherBookmarkChildRecord);

class BraveSyncServiceTest;

namespace bookmarks {
class BookmarkModel;
class BookmarkNode;
}  // namespace bookmarks

namespace brave_sync {
namespace prefs {
class Prefs;
}  // namespace prefs

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

class BraveProfileSyncServiceImpl
    : public BraveProfileSyncService,
      public BraveSyncService,
      public network::NetworkConnectionTracker::NetworkConnectionObserver,
      public SyncMessageHandler,
      public bookmarks::BookmarkModelObserver {
 public:
  explicit BraveProfileSyncServiceImpl(Profile* profile,
                                       InitParams init_params);

  ~BraveProfileSyncServiceImpl() override;

  // BraveSyncService implementation
  void OnSetupSyncHaveCode(const std::string& sync_words,
                           const std::string& device_name) override;
  void OnSetupSyncNewToSync(const std::string& device_name) override;
  void OnDeleteDevice(const std::string& device_id_v2) override;
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
                      const brave_sync::Uint8Array& device_id,
                      const std::string& device_id_v2) override;
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
  void OnCompactComplete(const std::string& category_name) override;
  void OnRecordsSent(
      const std::string& category_name,
      std::unique_ptr<brave_sync::RecordsList> records) override;

  // syncer::SyncService implementation
  int GetDisableReasons() const override;
  CoreAccountInfo GetAuthenticatedAccountInfo() const override;
  bool IsAuthenticatedAccountPrimary() const override;

  // NetworkConnectionTracker::NetworkConnectionObserver implementation.
  void OnConnectionChanged(network::mojom::ConnectionType type) override;

  // KeyedService implementation. This must be called exactly
  // once (before this object is destroyed).
  void Shutdown() override;

  // bookmarks::BookmarkModelObserver implementation
  void BookmarkModelLoaded(BookmarkModel* model, bool ids_reassigned) override;

  void BookmarkNodeMoved(BookmarkModel* model,
                         const BookmarkNode* old_parent,
                         size_t old_index,
                         const BookmarkNode* new_parent,
                         size_t new_index) override {}

  void BookmarkNodeAdded(BookmarkModel* model,
                         const BookmarkNode* parent,
                         size_t index) override {}

  void BookmarkNodeRemoved(
      BookmarkModel* model,
      const BookmarkNode* parent,
      size_t old_index,
      const BookmarkNode* node,
      const std::set<GURL>& no_longer_bookmarked) override {}

  void BookmarkNodeChanged(BookmarkModel* model,
                           const BookmarkNode* node) override {}

  void BookmarkNodeFaviconChanged(BookmarkModel* model,
                                  const BookmarkNode* node) override {}

  void BookmarkNodeChildrenReordered(BookmarkModel* model,
                                     const BookmarkNode* node) override {}

  void BookmarkAllUserNodesRemoved(
      BookmarkModel* model,
      const std::set<GURL>& removed_urls) override {}

#if BUILDFLAG(ENABLE_EXTENSIONS)
  BraveSyncClient* GetBraveSyncClient() override;
#endif

  static void AddNonClonedBookmarkKeys(BookmarkModel* model);

  bool IsBraveSyncEnabled() const override;

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
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, OnResetSyncWhenOffline);
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
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, SendCompact);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, SetSyncEnabled);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, SetSyncDisabled);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, IsSyncReadyOnNewProfile);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, SetThisDeviceCreatedTime);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, InitialFetchesStartWithZero);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest, DeviceIdV2Migration);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest,
                           DeviceIdV2MigrationDupDeviceId);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTestDelayedLoadModel,
                           OnSyncReadyModelNotYetLoaded);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest,
                           IsOtherBookmarksFolder);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest,
                           ProcessOtherBookmarksFolder);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest,
                           ProcessOtherBookmarksChildren);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest,
                           CheckOtherBookmarkRecord);
  FRIEND_TEST_ALL_PREFIXES(::BraveSyncServiceTest,
                           CheckOtherBookmarkChildRecord);

  friend class ::BraveSyncServiceTest;

  void SignalWaitableEvent();
  void FetchSyncRecords(const bool bookmarks,
                        const bool history,
                        const bool preferences,
                        int max_records);
  void FetchDevices();
  void SendCreateDevice();
  void SendDeleteDevice();
  void SendDeviceSyncRecord(const int action,
                            const std::string& device_name,
                            const std::string& device_id,
                            const std::string& device_id_v2,
                            const std::string& object_id);
  void OnResolvedPreferences(const brave_sync::RecordsList& records);
  void OnBraveSyncPrefsChanged(const std::string& pref);
  void NotifySyncSetupError(const std::string& error);
  void NotifySyncStateChanged();
  void NotifyHaveSyncWords(const std::string& sync_words);

  void ResetSyncInternal();

  void SetPermanentNodesOrder(const std::string& base_order);

  std::unique_ptr<jslib::SyncRecord> BookmarkNodeToSyncBookmark(
      const bookmarks::BookmarkNode* node);
  // These SyncEntityInfo is for legacy device who doesn't send meta info for
  // sync entity
  void SaveSyncEntityInfo(const jslib::SyncRecord* record);
  void LoadSyncEntityInfo(jslib::SyncRecord* record);

  bool IsOtherBookmarksFolder(const jslib::SyncRecord* record) const;
  // Handling "Other Bookmarks" remote records
  void ProcessOtherBookmarksFolder(const jslib::SyncRecord* record,
                                   bool* pass_to_syncer);
  // Handling direct children of "Other Bookmarks" remote records
  void ProcessOtherBookmarksChildren(jslib::SyncRecord* record);
  // Check and correct info before sending records
  void CheckOtherBookmarkRecord(jslib::SyncRecord* record);
  void CheckOtherBookmarkChildRecord(jslib::SyncRecord* record);

  void CreateResolveList(
      const std::vector<std::unique_ptr<jslib::SyncRecord>>& records,
      SyncRecordAndExistingList* records_and_existing_objects);
  std::unique_ptr<SyncRecordAndExistingList> PrepareResolvedPreferences(
      const RecordsList& records);

  void SendSyncRecords(const std::string& category_name,
                       RecordsListPtr records);
  void ResendSyncRecords(const std::string& category_name);

  void RecordSyncStateP3A() const;

  bool IsSQSReady() const;

  // Method to be run right after bookmark model will be loaded
  void OnSyncReadyBookmarksModelLoaded();

  static base::TimeDelta GetRetryExponentialWaitAmount(int retry_number);
  static std::vector<unsigned> GetExponentialWaitsForTests();
  static const std::vector<unsigned> kExponentialWaits;
  static const int kMaxSendRetries;
  static const int kCompactPeriodInDays = 7;
  static constexpr int GetCompactPeriodInDaysForTests() {
    return kCompactPeriodInDays;
  }

  std::unique_ptr<brave_sync::prefs::Prefs> brave_sync_prefs_;

  // True if we have received SyncReady from JS lib
  // This is used only to prevent out of sequence invocation of OnSaveInitData
  // and prevent double invocation of OnSyncReady
  bool brave_sync_ready_ = false;

  // Prevent two sequential calls OnSetupSyncHaveCode or OnSetupSyncNewToSync
  // while being initializing
  bool brave_sync_initializing_ = false;

  bool send_device_id_v2_update_ = false;

  Uint8Array seed_;

  brave_sync::GetRecordsCallback get_record_cb_;
  base::WaitableEvent* wevent_ = nullptr;

  // Registrar used to monitor the brave_profile prefs.
  PrefChangeRegistrar brave_pref_change_registrar_;

  bookmarks::BookmarkModel* model_ = nullptr;

  std::unique_ptr<BraveSyncClient> brave_sync_client_;

  std::unique_ptr<RecordsList> pending_received_records_;
  // Time when current device sent CREATE device record
  base::Time this_device_created_time_;

  bool pending_self_reset_ = false;

  bool is_model_loaded_observer_set_ = false;

  // Used to ensure that certain operations are performed on the sequence that
  // this object was created on.
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(BraveProfileSyncServiceImpl);
};
}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_PROFILE_SYNC_SERVICE_IMPL_H_
