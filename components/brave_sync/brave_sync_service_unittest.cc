/* Copyright 2016 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <set>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_sync/brave_profile_sync_service_impl.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/brave_sync_service.h"
#include "brave/components/brave_sync/brave_sync_service_observer.h"
#include "brave/components/brave_sync/client/brave_sync_client_impl.h"
#include "brave/components/brave_sync/client/client_ext_impl_data.h"
#include "brave/components/brave_sync/jslib_const.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/settings.h"
#include "brave/components/brave_sync/sync_devices.h"
#include "brave/components/brave_sync/test_util.h"
#include "brave/components/brave_sync/tools.h"
#include "brave/components/brave_sync/values_conv.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/test/test_bookmark_client.h"
#include "components/prefs/pref_service.h"
#include "components/sync/base/pref_names.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/test/browser_task_environment.h"
#include "net/base/network_interfaces.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveSyncServiceTest.*

// BraveSyncClient::methods
// Name                     | Covered
//------------------------------------
// SetSyncToBrowserHandler  |
// GetSyncToBrowserHandler  |
// SendGotInitData          | OnGetInitData
// SendFetchSyncRecords     |
// SendResolveSyncRecords   |
// SendSyncRecords          |
// SendDeleteSyncUser       |
// SendDeleteSyncCategory   |
// SendGetBookmarksBaseOrder|
// OnExtensionInitialized   |

// BraveSyncService::methods
// Name                      | Covered
//-------------------------------------
// OnSetupSyncHaveCode       | +
// OnSetupSyncNewToSync      | +
// OnDeleteDevice            | +
// OnResetSync               | +
// GetSettingsAndDevices     | +
// GetSyncWords              | +
// GetSeed                   | +
// OnSetSyncEnabled          | +
// OnSetSyncBookmarks        | +
// OnSetSyncBrowsingHistory  | +
// OnSetSyncSavedSiteSettings| +
// AddObserver               | +, SetUp
// RemoveObserver            | +, Teardown
// GetSyncClient             | +, SetUp

// BraveSyncService  SyncMessageHandler overrides
// Name                      | Covered
//-------------------------------------
// BackgroundSyncStarted     | N/A
// BackgroundSyncStopped     | +
// OnSyncDebug               | +
// OnSyncSetupError          | Need UI handler
// OnGetInitData             | +
// OnSaveInitData            | BraveSyncServiceTest.GetSeed
// OnSyncReady               | +
// OnGetExistingObjects      | +
// OnResolvedSyncRecords     | N/A
// OnDeletedSyncUser         | N/A
// OnDeleteSyncSiteSettings  | N/A
// OnSaveBookmarksBaseOrder  | +
// OnResolvedHistorySites    | N/A
// OnResolvedPreferences     | BraveSyncServiceTest.OnDeleteDevice,
//                           | BraveSyncServiceTest.OnResetSync
// OnBraveSyncPrefsChanged   | +

using bookmarks::BookmarkModel;
using brave_sync::BraveProfileSyncServiceImpl;
using brave_sync::BraveSyncService;
using brave_sync::BraveSyncServiceObserver;
using brave_sync::MockBraveSyncClient;
using brave_sync::RecordsList;
using brave_sync::SimpleBookmarkSyncRecord;
using brave_sync::SimpleDeviceRecord;
using brave_sync::jslib::SyncRecord;
using testing::_;
using testing::AtLeast;

namespace {

const bookmarks::BookmarkNode* GetSingleNodeByUrl(
    bookmarks::BookmarkModel* model,
    const std::string& url) {
  std::vector<const bookmarks::BookmarkNode*> nodes;
  model->GetNodesByURL(GURL(url), &nodes);
  size_t nodes_size = nodes.size();
  CHECK_EQ(nodes_size, 1u);
  const bookmarks::BookmarkNode* node = nodes.at(0);
  return node;
}

bool DevicesContains(brave_sync::SyncDevices* devices,
                     const std::string& id,
                     const std::string& name) {
  DCHECK(devices);
  for (const auto& device : devices->devices_) {
    if (device.device_id_ == id && device.name_ == name) {
      return true;
    }
  }
  return false;
}

MATCHER_P2(ContainsDeviceRecord,
           action,
           name,
           "contains device sync record with params") {
  for (const auto& record : arg) {
    if (record->has_device()) {
      const auto& device = record->GetDevice();
      if (record->action == action && device.name == name) {
        return true;
      }
    }
  }
  return false;
}

base::TimeDelta g_overridden_time_delta;
base::Time g_overridden_now;

std::unique_ptr<base::subtle::ScopedTimeClockOverrides> OverrideForTimeDelta(
    base::TimeDelta overridden_time_delta,
    const base::Time& now = base::subtle::TimeNowIgnoringOverride()) {
  g_overridden_time_delta = overridden_time_delta;
  g_overridden_now = now;
  return std::make_unique<base::subtle::ScopedTimeClockOverrides>(
      []() { return g_overridden_now + g_overridden_time_delta; }, nullptr,
      nullptr);
}

}  // namespace

class MockBraveSyncServiceObserver : public BraveSyncServiceObserver {
 public:
  MockBraveSyncServiceObserver() {}

  MOCK_METHOD2(OnSyncSetupError, void(BraveSyncService*, const std::string&));
  MOCK_METHOD1(OnSyncStateChanged, void(BraveSyncService*));
  MOCK_METHOD2(OnHaveSyncWords, void(BraveSyncService*, const std::string&));
};

class BraveSyncServiceTest : public testing::Test {
 public:
  BraveSyncServiceTest() {}
  ~BraveSyncServiceTest() override {}

 protected:
  void SetUp() override {
    EXPECT_TRUE(temp_dir_.CreateUniqueTempDir());
    // register the factory

    profile_ = brave_sync::CreateBraveSyncProfile(temp_dir_.GetPath());
    EXPECT_TRUE(profile_.get() != NULL);

    sync_prefs_ = std::make_unique<syncer::SyncPrefs>(profile_->GetPrefs());

    // TODO(bridiver) - this is temporary until some changes are made to
    // to bookmark_change_processor to allow `set_for_testing` like
    // BraveSyncClient
    BookmarkModelFactory::GetInstance()->SetTestingFactory(
        profile(),
        base::BindRepeating(&brave_sync::BuildFakeBookmarkModelForTests));

    model_ = BookmarkModelFactory::GetForBrowserContext(
        Profile::FromBrowserContext(profile_.get()));
    EXPECT_NE(model(), nullptr);

    sync_client_ = new MockBraveSyncClient();
    brave_sync::BraveSyncClientImpl::set_for_testing(sync_client_);

    sync_service_ = static_cast<BraveProfileSyncServiceImpl*>(
        ProfileSyncServiceFactory::GetAsProfileSyncServiceForProfile(
            profile()));

    EXPECT_EQ(sync_client_, sync_service_->GetBraveSyncClient());

    observer_.reset(new MockBraveSyncServiceObserver);
    sync_service_->BraveSyncService::AddObserver(observer_.get());
    EXPECT_TRUE(sync_service_ != NULL);
  }

  void TearDown() override {
    sync_service_->BraveSyncService::RemoveObserver(observer_.get());
    // this will also trigger a shutdown of the brave sync service
    sync_service_->Shutdown();
    sync_prefs_.reset();
    profile_.reset();
  }

  Profile* profile() { return profile_.get(); }
  BraveProfileSyncServiceImpl* sync_service() { return sync_service_; }
  MockBraveSyncClient* sync_client() { return sync_client_; }
  BookmarkModel* model() { return model_; }
  MockBraveSyncServiceObserver* observer() { return observer_.get(); }
  brave_sync::prefs::Prefs* brave_sync_prefs() {
    return sync_service_->brave_sync_prefs_.get();
  }
  syncer::SyncPrefs* sync_prefs() { return sync_prefs_.get(); }

 private:
  // Need this as a very first member to run tests in UI thread
  // When this is set, class should not install any other MessageLoops, like
  // base::test::ScopedTaskEnvironment
  content::TestBrowserThreadBundle thread_bundle_;

  std::unique_ptr<Profile> profile_;
  BraveProfileSyncServiceImpl* sync_service_;
  MockBraveSyncClient* sync_client_;
  BookmarkModel* model_;  // Not owns
  std::unique_ptr<syncer::SyncPrefs> sync_prefs_;
  std::unique_ptr<MockBraveSyncServiceObserver> observer_;

  base::ScopedTempDir temp_dir_;
};

TEST_F(BraveSyncServiceTest, SetSyncEnabled) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged);
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  EXPECT_FALSE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));
  sync_service()->OnSetSyncEnabled(true);
  EXPECT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));
  EXPECT_FALSE(sync_service()->IsBraveSyncInitialized());
  EXPECT_FALSE(sync_service()->IsBraveSyncConfigured());
}

TEST_F(BraveSyncServiceTest, SetSyncDisabled) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  sync_service()->OnSetSyncEnabled(true);
  EXPECT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));

  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  sync_service()->OnSetSyncEnabled(false);
  EXPECT_FALSE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));
  EXPECT_FALSE(sync_service()->IsBraveSyncInitialized());
  EXPECT_FALSE(sync_service()->IsBraveSyncConfigured());
}

TEST_F(BraveSyncServiceTest, IsSyncConfiguredOnNewProfile) {
  EXPECT_FALSE(sync_service()->IsBraveSyncConfigured());
}

TEST_F(BraveSyncServiceTest, IsSyncInitializedOnNewProfile) {
  EXPECT_FALSE(sync_service()->IsBraveSyncInitialized());
}

TEST_F(BraveSyncServiceTest, OnSetupSyncHaveCode) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged);
  // Expecting sync state changed twice: for enabled state and for device name
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(2);
  sync_service()->OnSetupSyncHaveCode("word1 word2 word3", "test_device");
  EXPECT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));
}

TEST_F(BraveSyncServiceTest, OnSetupSyncHaveCodeEmptyDeviceName) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged);
  // Expecting sync state changed twice: for enabled state and for device name
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(2);
  sync_service()->OnSetupSyncHaveCode("word1 word2 word3", "");
  EXPECT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));
  EXPECT_EQ(
      profile()->GetPrefs()->GetString(brave_sync::prefs::kSyncDeviceName),
      net::GetHostName());
}

TEST_F(BraveSyncServiceTest, OnSetupSyncNewToSync) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged);
  // Expecting sync state changed twice: for enabled state and for device name
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(2);
  sync_service()->OnSetupSyncNewToSync("test_device");
  EXPECT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));
}

TEST_F(BraveSyncServiceTest, OnSetupSyncNewToSyncEmptyDeviceName) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged);
  // Expecting sync state changed twice: for enabled state and for device name
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(2);
  sync_service()->OnSetupSyncNewToSync("");
  EXPECT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));
  EXPECT_EQ(
      profile()->GetPrefs()->GetString(brave_sync::prefs::kSyncDeviceName),
      net::GetHostName());
}

TEST_F(BraveSyncServiceTest, GetSettingsAndDevices) {
  // The test absorbs OnSetupSyncNewToSync test
  auto callback1 =
      base::BindRepeating([](std::unique_ptr<brave_sync::Settings> settings,
                             std::unique_ptr<brave_sync::SyncDevices> devices) {
        EXPECT_TRUE(settings->this_device_name_.empty());
        EXPECT_TRUE(settings->this_device_id_.empty());
        EXPECT_FALSE(settings->sync_configured_);
        EXPECT_FALSE(settings->sync_this_device_);
        EXPECT_FALSE(settings->sync_bookmarks_);
        EXPECT_FALSE(settings->sync_settings_);
        EXPECT_FALSE(settings->sync_history_);
        EXPECT_EQ(devices->size(), 0u);
      });

  sync_service()->GetSettingsAndDevices(callback1);
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged);
  // Expecting sync state changed twice: for enabled state and for device name
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(2);
  sync_service()->OnSetupSyncNewToSync("test_device");
  EXPECT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));
  auto callback2 =
      base::BindRepeating([](std::unique_ptr<brave_sync::Settings> settings,
                             std::unique_ptr<brave_sync::SyncDevices> devices) {
        // Other fields may be switched later
        EXPECT_EQ(settings->this_device_name_, "test_device");
        EXPECT_TRUE(settings->sync_this_device_);
      });
  sync_service()->GetSettingsAndDevices(callback2);
}

TEST_F(BraveSyncServiceTest, GetSyncWords) {
  brave_sync_prefs()->SetSeed(
      "247,124,20,15,38,187,78,131,12,125,165,67,221,207,143,120,166,118,77,"
      "107,128,115,21,66,254,154,99,38,205,220,244,245");
  std::string words =
      "wash thing adult estate reject dose cradle regret duck unveil toilet "
      "vanish guess chase puppy attack best blood pledge shock holiday unveil "
      "stable ring";
  EXPECT_CALL(*observer(), OnHaveSyncWords(sync_service(), words)).Times(1);
  sync_service()->GetSyncWords();
}

TEST_F(BraveSyncServiceTest, SyncSetupError) {
  EXPECT_CALL(*observer(), OnSyncSetupError(sync_service(), _)).Times(1);
  sync_service()->OnSetupSyncHaveCode("", "");
}

TEST_F(BraveSyncServiceTest, GetSeed) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged);
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service()))
      .Times(AtLeast(2));
  sync_service()->OnSetupSyncNewToSync("test_device");
  EXPECT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));

  // Service gets seed from client via BraveSyncServiceImpl::OnSaveInitData
  const auto binary_seed = brave_sync::Uint8Array(16, 77);

  EXPECT_TRUE(brave_sync_prefs()->GetPrevSeed().empty());
  sync_service()->OnSaveInitData(binary_seed, {0});
  std::string expected_seed = brave_sync::StrFromUint8Array(binary_seed);
  EXPECT_EQ(sync_service()->GetSeed(), expected_seed);
  EXPECT_TRUE(brave_sync_prefs()->GetPrevSeed().empty());
}

TEST_F(BraveSyncServiceTest, OnDeleteDevice) {
  RecordsList records;
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "1", "device1"));
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "2", "device2"));
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "3", "device3"));
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  sync_service()->OnResolvedPreferences(records);

  brave_sync_prefs()->SetThisDeviceId("1");
  auto devices = brave_sync_prefs()->GetSyncDevices();

  EXPECT_TRUE(DevicesContains(devices.get(), "1", "device1"));
  EXPECT_TRUE(DevicesContains(devices.get(), "2", "device2"));
  EXPECT_TRUE(DevicesContains(devices.get(), "3", "device3"));

  EXPECT_CALL(*sync_client(),
              SendSyncRecords("PREFERENCES",
                              ContainsDeviceRecord(SyncRecord::Action::A_DELETE,
                                                   "device3")))
      .Times(1);
  sync_service()->OnDeleteDevice("3");

  RecordsList resolved_records;
  auto resolved_record = SyncRecord::Clone(*records.at(2));
  resolved_record->action = SyncRecord::Action::A_DELETE;
  resolved_records.push_back(std::move(resolved_record));

  // Emulate we have twice the same delete device record to ensure we
  // don't have failed DCHECK in debug build
  auto resolved_record2 = SyncRecord::Clone(*records.at(2));
  resolved_record2->action = SyncRecord::Action::A_DELETE;
  resolved_records.push_back(std::move(resolved_record2));

  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  sync_service()->OnResolvedPreferences(resolved_records);

  auto devices_final = brave_sync_prefs()->GetSyncDevices();
  EXPECT_TRUE(DevicesContains(devices_final.get(), "1", "device1"));
  EXPECT_TRUE(DevicesContains(devices_final.get(), "2", "device2"));
  EXPECT_FALSE(DevicesContains(devices_final.get(), "3", "device3"));
}

TEST_F(BraveSyncServiceTest, OnDeleteDeviceWhenOneDevice) {
  brave_sync_prefs()->SetThisDeviceId("1");
  RecordsList records;
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "1", "device1"));
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "2", "device2"));
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  sync_service()->OnResolvedPreferences(records);

  auto devices = brave_sync_prefs()->GetSyncDevices();

  EXPECT_TRUE(DevicesContains(devices.get(), "1", "device1"));
  EXPECT_TRUE(DevicesContains(devices.get(), "2", "device2"));

  EXPECT_CALL(*sync_client(), SendSyncRecords).Times(1);
  sync_service()->OnDeleteDevice("2");

  RecordsList resolved_records;
  auto resolved_record = SyncRecord::Clone(*records.at(1));
  resolved_record->action = SyncRecord::Action::A_DELETE;
  resolved_records.push_back(std::move(resolved_record));
  // Expecting to be called one time to set the new devices list
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);

  EXPECT_CALL(*sync_client(), SendSyncRecords).Times(1);

  sync_service()->OnResolvedPreferences(resolved_records);

  auto devices_semi_final = brave_sync_prefs()->GetSyncDevices();
  EXPECT_FALSE(DevicesContains(devices_semi_final.get(), "2", "device2"));
  EXPECT_TRUE(DevicesContains(devices_semi_final.get(), "1", "device1"));

  // Enabled->Disabled
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);

  // Emulate sending DELETE for this device
  RecordsList resolved_records2;
  auto resolved_record2 = SyncRecord::Clone(*records.at(0));
  resolved_record2->action = SyncRecord::Action::A_DELETE;
  resolved_records2.push_back(std::move(resolved_record2));
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(3);

  sync_service()->OnResolvedPreferences(resolved_records2);

  auto devices_final = brave_sync_prefs()->GetSyncDevices();
  EXPECT_FALSE(DevicesContains(devices_final.get(), "1", "device1"));
  EXPECT_FALSE(DevicesContains(devices_final.get(), "2", "device2"));
  EXPECT_FALSE(sync_service()->IsBraveSyncConfigured());
}

TEST_F(BraveSyncServiceTest, OnDeleteDeviceWhenSelfDeleted) {
  brave_sync_prefs()->SetThisDeviceId("1");
  RecordsList records;
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "1", "device1"));
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "2", "device2"));
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  sync_service()->OnResolvedPreferences(records);

  auto devices = brave_sync_prefs()->GetSyncDevices();

  EXPECT_TRUE(DevicesContains(devices.get(), "1", "device1"));
  EXPECT_TRUE(DevicesContains(devices.get(), "2", "device2"));

  EXPECT_CALL(*sync_client(),
              SendSyncRecords("PREFERENCES",
                              ContainsDeviceRecord(SyncRecord::Action::A_DELETE,
                                                   "device1")))
      .Times(1);
  sync_service()->OnDeleteDevice("1");

  // Enabled->Disabled
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);

  RecordsList resolved_records;
  auto resolved_record = SyncRecord::Clone(*records.at(0));
  resolved_record->action = SyncRecord::Action::A_DELETE;
  resolved_records.push_back(std::move(resolved_record));
  // If you have to modify .Times(3) to another value, double re-check
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(3);
  sync_service()->OnResolvedPreferences(resolved_records);

  auto devices_final = brave_sync_prefs()->GetSyncDevices();
  EXPECT_FALSE(DevicesContains(devices_final.get(), "1", "device1"));
  EXPECT_FALSE(DevicesContains(devices_final.get(), "2", "device2"));

  EXPECT_FALSE(sync_service()->IsBraveSyncConfigured());
}

TEST_F(BraveSyncServiceTest, OnResetSync) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(AtLeast(1));
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service()))
      .Times(AtLeast(3));
  sync_service()->OnSetupSyncNewToSync("this_device");
  EXPECT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));
  brave_sync_prefs()->SetThisDeviceId("0");

  RecordsList records;
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "0", "this_device"));
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "1", "device1"));

  sync_service()->OnResolvedPreferences(records);

  auto devices = brave_sync_prefs()->GetSyncDevices();

  EXPECT_TRUE(DevicesContains(devices.get(), "0", "this_device"));
  EXPECT_TRUE(DevicesContains(devices.get(), "1", "device1"));

  EXPECT_CALL(*sync_client(),
              SendSyncRecords("PREFERENCES",
                              ContainsDeviceRecord(SyncRecord::Action::A_DELETE,
                                                   "this_device")))
      .Times(1);

  sync_service()->OnResetSync();
  RecordsList resolved_records;
  auto resolved_record = SyncRecord::Clone(*records.at(0));
  resolved_record->action = SyncRecord::Action::A_DELETE;
  resolved_records.push_back(std::move(resolved_record));
  sync_service()->OnResolvedPreferences(resolved_records);

  auto devices_final = brave_sync_prefs()->GetSyncDevices();
  EXPECT_FALSE(DevicesContains(devices_final.get(), "0", "this_device"));
  EXPECT_FALSE(DevicesContains(devices_final.get(), "1", "device1"));

  EXPECT_TRUE(profile()
                  ->GetPrefs()
                  ->GetString(brave_sync::prefs::kSyncDeviceId)
                  .empty());
  EXPECT_TRUE(
      profile()->GetPrefs()->GetString(brave_sync::prefs::kSyncSeed).empty());
  EXPECT_TRUE(profile()
                  ->GetPrefs()
                  ->GetString(brave_sync::prefs::kSyncDeviceName)
                  .empty());
  EXPECT_FALSE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));
  EXPECT_FALSE(profile()->GetPrefs()->GetBoolean(
      brave_sync::prefs::kSyncBookmarksEnabled));
  EXPECT_TRUE(profile()
                  ->GetPrefs()
                  ->GetString(brave_sync::prefs::kSyncBookmarksBaseOrder)
                  .empty());
  EXPECT_FALSE(profile()->GetPrefs()->GetBoolean(
      brave_sync::prefs::kSyncSiteSettingsEnabled));
  EXPECT_FALSE(profile()->GetPrefs()->GetBoolean(
      brave_sync::prefs::kSyncHistoryEnabled));
  EXPECT_TRUE(profile()
                  ->GetPrefs()
                  ->GetTime(brave_sync::prefs::kSyncLatestRecordTime)
                  .is_null());
  EXPECT_TRUE(profile()
                  ->GetPrefs()
                  ->GetTime(brave_sync::prefs::kSyncLastFetchTime)
                  .is_null());
  EXPECT_TRUE(profile()
                  ->GetPrefs()
                  ->GetString(brave_sync::prefs::kSyncDeviceList)
                  .empty());
  EXPECT_EQ(
      profile()->GetPrefs()->GetString(brave_sync::prefs::kSyncApiVersion),
      "0");

  EXPECT_FALSE(sync_service()->IsBraveSyncInitialized());
  EXPECT_FALSE(sync_service()->IsBraveSyncConfigured());

  EXPECT_FALSE(sync_prefs()->IsSyncRequested());
  EXPECT_FALSE(
      profile()->GetPrefs()->GetBoolean(syncer::prefs::kSyncBookmarks));
}

TEST_F(BraveSyncServiceTest, OnSetSyncBookmarks) {
  EXPECT_FALSE(profile()->GetPrefs()->GetBoolean(
      brave_sync::prefs::kSyncBookmarksEnabled));
  EXPECT_CALL(*observer(), OnSyncStateChanged).Times(1);
  sync_service()->OnSetSyncBookmarks(true);
  EXPECT_TRUE(profile()->GetPrefs()->GetBoolean(syncer::prefs::kSyncBookmarks));
  EXPECT_TRUE(profile()->GetPrefs()->GetBoolean(
      brave_sync::prefs::kSyncBookmarksEnabled));
  EXPECT_CALL(*observer(), OnSyncStateChanged).Times(1);
  sync_service()->OnSetSyncBookmarks(false);
  EXPECT_FALSE(profile()->GetPrefs()->GetBoolean(
      brave_sync::prefs::kSyncBookmarksEnabled));
  EXPECT_FALSE(
      profile()->GetPrefs()->GetBoolean(syncer::prefs::kSyncBookmarks));
  EXPECT_CALL(*observer(), OnSyncStateChanged).Times(0);
  sync_service()->OnSetSyncBookmarks(false);
}

TEST_F(BraveSyncServiceTest, OnSetSyncBrowsingHistory) {
  EXPECT_FALSE(profile()->GetPrefs()->GetBoolean(
      brave_sync::prefs::kSyncHistoryEnabled));
  EXPECT_CALL(*observer(), OnSyncStateChanged).Times(1);
  sync_service()->OnSetSyncBrowsingHistory(true);
  EXPECT_TRUE(profile()->GetPrefs()->GetBoolean(
      brave_sync::prefs::kSyncHistoryEnabled));
  EXPECT_CALL(*observer(), OnSyncStateChanged).Times(1);
  sync_service()->OnSetSyncBrowsingHistory(false);
  EXPECT_FALSE(profile()->GetPrefs()->GetBoolean(
      brave_sync::prefs::kSyncHistoryEnabled));
}

TEST_F(BraveSyncServiceTest, OnSetSyncSavedSiteSettings) {
  EXPECT_FALSE(profile()->GetPrefs()->GetBoolean(
      brave_sync::prefs::kSyncSiteSettingsEnabled));
  EXPECT_CALL(*observer(), OnSyncStateChanged).Times(1);
  sync_service()->OnSetSyncSavedSiteSettings(true);
  EXPECT_TRUE(profile()->GetPrefs()->GetBoolean(
      brave_sync::prefs::kSyncSiteSettingsEnabled));
  EXPECT_CALL(*observer(), OnSyncStateChanged).Times(1);
  sync_service()->OnSetSyncSavedSiteSettings(false);
  EXPECT_FALSE(profile()->GetPrefs()->GetBoolean(
      brave_sync::prefs::kSyncSiteSettingsEnabled));
}

TEST_F(BraveSyncServiceTest, OnGetInitData) {
  EXPECT_CALL(*sync_client(), SendGotInitData).Times(1);
  sync_service()->OnGetInitData("v1.4.2");
}

TEST_F(BraveSyncServiceTest, OnSaveBookmarksBaseOrder) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);
  // kSyncEnabled and kSyncBookmarksEnabled
  EXPECT_CALL(*observer(), OnSyncStateChanged).Times(2);
  sync_service()->OnSetSyncEnabled(true);
  sync_service()->OnSaveBookmarksBaseOrder("1.1.");
  EXPECT_EQ(profile()->GetPrefs()->GetString(
                brave_sync::prefs::kSyncBookmarksBaseOrder),
            "1.1.");
  // Permanent node order
  std::string order;
  model()->bookmark_bar_node()->GetMetaInfo("order", &order);
  EXPECT_EQ(order, "1.1.1");
  order.clear();
  model()->other_node()->GetMetaInfo("order", &order);
  EXPECT_EQ(order, "1.1.2");
  EXPECT_EQ(brave_sync_prefs()->GetMigratedBookmarksVersion(), 2);
}

TEST_F(BraveSyncServiceTest, OnBraveSyncPrefsChanged) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);
  EXPECT_CALL(*observer(), OnSyncStateChanged);
  sync_service()->OnBraveSyncPrefsChanged(brave_sync::prefs::kSyncEnabled);
}

TEST_F(BraveSyncServiceTest, StartSyncNonDeviceRecords) {
  EXPECT_FALSE(sync_service()->IsBraveSyncInitialized());
  sync_service()->OnSetSyncEnabled(true);
  profile()->GetPrefs()->SetString(brave_sync::prefs::kSyncBookmarksBaseOrder,
                                   "1.1.");
  profile()->GetPrefs()->SetBoolean(brave_sync::prefs::kSyncBookmarksEnabled,
                                    true);
  EXPECT_FALSE(
      profile()->GetPrefs()->GetBoolean(syncer::prefs::kSyncBookmarks));
  brave_sync_prefs()->SetThisDeviceId("1");
  RecordsList records;
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "1", "device1"));
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "2", "device2"));
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  sync_service()->OnResolvedPreferences(records);
  EXPECT_TRUE(
      !brave_sync::tools::IsTimeEmpty(sync_service()->chain_created_time_));
}

TEST_F(BraveSyncServiceTest, OnSyncReadyNewToSync) {
  sync_prefs()->SetSyncRequested(false);
  EXPECT_CALL(*observer(), OnSyncStateChanged);
  EXPECT_CALL(*sync_client(), SendGetBookmarksBaseOrder).Times(1);
  sync_service()->OnSyncReady();

  // simulate OnSaveBookmarksBaseOrder
  profile()->GetPrefs()->SetString(brave_sync::prefs::kSyncBookmarksBaseOrder,
                                   "1.1.");
  sync_service()->OnSyncReady();
  EXPECT_TRUE(sync_prefs()->IsSyncRequested());

  // We want to have Chromium syncer bookmarks be enabled from begin to avoid
  // reaching BookmarkModelAssociator::AssociateModels
  EXPECT_TRUE(profile()->GetPrefs()->GetBoolean(syncer::prefs::kSyncBookmarks));
}

TEST_F(BraveSyncServiceTest, OnGetExistingObjects) {
  EXPECT_CALL(*sync_client(), SendResolveSyncRecords).Times(1);

  auto records = std::make_unique<RecordsList>();
  sync_service()->OnGetExistingObjects(brave_sync::jslib_const::kBookmarks,
                                       std::move(records), base::Time(), false);
}

TEST_F(BraveSyncServiceTest, GetPreferredDataTypes) {
  // Make sure GetPreferredDataTypes contains DEVICE_INFO which is needed for
  // brave device record polling
  EXPECT_TRUE(sync_service()->GetPreferredDataTypes().Has(syncer::DEVICE_INFO));
}

TEST_F(BraveSyncServiceTest, GetDisableReasons) {
  sync_prefs()->SetManagedForTest(true);
  EXPECT_EQ(sync_service()->GetDisableReasons(),
            syncer::SyncService::DISABLE_REASON_ENTERPRISE_POLICY |
                syncer::SyncService::DISABLE_REASON_USER_CHOICE);
  sync_service()->OnSetSyncEnabled(true);
  EXPECT_EQ(sync_service()->GetDisableReasons(),
            syncer::SyncService::DISABLE_REASON_ENTERPRISE_POLICY |
                syncer::SyncService::DISABLE_REASON_USER_CHOICE);
  brave_sync_prefs()->SetMigratedBookmarksVersion(2);
  EXPECT_EQ(sync_service()->GetDisableReasons(),
            syncer::SyncService::DISABLE_REASON_NONE);
  sync_service()->OnSetSyncEnabled(false);
  EXPECT_TRUE(sync_service()->HasDisableReason(
      syncer::SyncService::DISABLE_REASON_ENTERPRISE_POLICY));
}

TEST_F(BraveSyncServiceTest, OnSetupSyncHaveCode_Reset_SetupAgain) {
  EXPECT_FALSE(sync_service()->GetResettingForTest());
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);
  // Expecting sync state changed twice: for enabled state and for device name
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(2);
  sync_service()->OnSetupSyncHaveCode("word1 word2 word3", "test_device");
  EXPECT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));

  brave_sync::Uint8Array seed = {1, 2,  3,  4,  5,  6,  7,  8,
                                 9, 10, 11, 12, 13, 14, 15, 16};
  brave_sync::Uint8Array device_id = {0};
  sync_service()->OnSaveInitData(seed, device_id);

  RecordsList records;
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "0", "this_device"));
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  sync_service()->OnResolvedPreferences(records);

  auto devices = brave_sync_prefs()->GetSyncDevices();

  ASSERT_EQ(devices->size(), 1u);
  EXPECT_TRUE(DevicesContains(devices.get(), "0", "this_device"));
  EXPECT_CALL(*sync_client(),
              SendSyncRecords("PREFERENCES",
                              ContainsDeviceRecord(SyncRecord::Action::A_DELETE,
                                                   "this_device")))
      .Times(1);
  sync_service()->OnResetSync();
  // Here we have marked service as in resetting state
  // Actual kSyncEnabled will go to false after receiving confirmation of
  // this_device DELETE
  EXPECT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));

  EXPECT_TRUE(sync_service()->GetResettingForTest());

  // OnSyncStateChanged actually is invoked 9 times:
  // ForceCompleteReset
  //    ResetSyncInternal
  //        brave_sync::Prefs::Clear()
  //            device_name
  //            enabled => false
  //            bookmarks_enabled
  //            site_settings_enabled
  //            history_enabled
  //            device_list
  //       enabled => false
  // OnSetupSyncHaveCode()
  //     device_name
  //     enabled => true

  // OnSyncEnabledChanged is actually invoked 3 times:
  // see preference enabled in abobe list
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service()))
      .Times(AtLeast(1));
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(AtLeast(1));
  sync_service()->OnSetupSyncHaveCode("word1 word2 word5", "test_device");
  EXPECT_FALSE(sync_service()->GetResettingForTest());

  EXPECT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));
}

TEST_F(BraveSyncServiceTest, ExponentialResend) {
  bookmarks::AddIfNotBookmarked(model(), GURL("https://a.com/"),
                                base::ASCIIToUTF16("A.com"));
  // Explicitly set sync_timestamp, object_id and order because it is supposed
  // to be set in syncer
  auto* node = GetSingleNodeByUrl(model(), "https://a.com/");
  model()->SetNodeMetaInfo(node, "sync_timestamp",
                           std::to_string(base::Time::Now().ToJsTime()));
  const char* record_a_object_id =
      "121, 194, 37, 61, 199, 11, 166, 234, "
      "214, 197, 45, 215, 241, 206, 219, 130";
  model()->SetNodeMetaInfo(node, "object_id", record_a_object_id);
  const char* record_a_order = "1.1.1.1";
  model()->SetNodeMetaInfo(node, "order", record_a_order);

  brave_sync_prefs()->SetThisDeviceId("1");
  std::unique_ptr<RecordsList> records = std::make_unique<RecordsList>();
  records->push_back(SimpleBookmarkSyncRecord(
      SyncRecord::Action::A_CREATE, record_a_object_id, "https://a.com/",
      "A.com - title", record_a_order, "",
      brave_sync_prefs()->GetThisDeviceId()));

  EXPECT_CALL(*sync_client(), SendSyncRecords("BOOKMARKS", _)).Times(1);
  sync_service()->SendSyncRecords("BOOKMARKS", std::move(records));

  EXPECT_EQ(brave_sync_prefs()->GetRecordsToResend().size(), 1u);
  const base::DictionaryValue* meta =
      brave_sync_prefs()->GetRecordToResendMeta(record_a_object_id);
  int send_retry_number = -1;
  meta->GetInteger("send_retry_number", &send_retry_number);
  EXPECT_EQ(send_retry_number, 0);
  double sync_timestamp = -1;
  meta->GetDouble("sync_timestamp", &sync_timestamp);
  EXPECT_NE(sync_timestamp, -1);

  int expected_send_retry_number = 0;

  static const std::vector<unsigned> exponential_waits =
      brave_sync::BraveProfileSyncServiceImpl::GetExponentialWaitsForTests();
  const int max_send_retries = exponential_waits.size() - 1;
  std::set<int> should_sent_at;
  size_t current_sum = 0;
  for (size_t j = 0; j < exponential_waits.size(); ++j) {
    current_sum += exponential_waits[j];
    should_sent_at.insert(current_sum);
  }
  should_sent_at.insert(current_sum + exponential_waits.back());
  auto contains = [](const std::set<int>& set, int val) {
    return set.find(val) != set.end();
  };
  // Following statemets are correct only if
  // kExponentialWaits is {10, 20, 40, 80}
  EXPECT_TRUE(contains(should_sent_at, 10));
  EXPECT_TRUE(contains(should_sent_at, 30));
  EXPECT_TRUE(contains(should_sent_at, 70));
  EXPECT_TRUE(contains(should_sent_at, 150));
  // emulate the wait time after reaching maximum retry
  EXPECT_TRUE(contains(should_sent_at, 230));
  for (size_t i = 0; i <= 231 + 1; ++i) {
    auto time_override =
        OverrideForTimeDelta(base::TimeDelta::FromMinutes(i));
    bool is_send_expected = contains(should_sent_at, i);
    int expect_call_times = is_send_expected ? 1 : 0;
    EXPECT_CALL(*sync_client(), SendSyncRecords("BOOKMARKS", _))
        .Times(expect_call_times);
    sync_service()->ResendSyncRecords("BOOKMARKS");

    if (is_send_expected) {
      if (++expected_send_retry_number > max_send_retries)
        expected_send_retry_number = max_send_retries;
      send_retry_number = -1;
      meta = brave_sync_prefs()->GetRecordToResendMeta(record_a_object_id);
      meta->GetInteger("send_retry_number", &send_retry_number);
      EXPECT_EQ(send_retry_number, expected_send_retry_number);
    }
  }

  // resolve to confirm records
  RecordsList records_to_resolve;
  records_to_resolve.push_back(SimpleBookmarkSyncRecord(
      SyncRecord::Action::A_CREATE, record_a_object_id, "https://a.com/",
      "A.com", "1.1.1.1", "", brave_sync_prefs()->GetThisDeviceId()));
  auto timestamp_resolve = base::Time::Now();
  records_to_resolve.at(0)->syncTimestamp = timestamp_resolve;
  brave_sync::SyncRecordAndExistingList records_and_existing_objects;
  sync_service()->CreateResolveList(records_to_resolve,
                                    &records_and_existing_objects);

  EXPECT_EQ(brave_sync_prefs()->GetRecordsToResend().size(), 0u);
  EXPECT_EQ(brave_sync_prefs()->GetRecordToResendMeta(record_a_object_id),
            nullptr);
}

TEST_F(BraveSyncServiceTest, GetDevicesWithFetchSyncRecords) {
  using brave_sync::jslib_const::kPreferences;

  // Expecting SendFetchSyncRecords will be invoked
  EXPECT_EQ(brave_sync_prefs()->GetLastFetchTime(), base::Time());
  EXPECT_EQ(brave_sync_prefs()->GetLatestDeviceRecordTime(), base::Time());
  EXPECT_CALL(*sync_client(), SendFetchSyncRecords(_, base::Time(), _))
      .Times(1);
  sync_service()->FetchDevices();

  // Emulate we received this device
  brave_sync_prefs()->SetThisDeviceId("1");
  auto records = std::make_unique<brave_sync::RecordsList>();
  records->push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "1", "device1"));
  auto device1_timestamp = records->back()->syncTimestamp;
  EXPECT_CALL(*sync_client(), SendResolveSyncRecords(kPreferences, _)).Times(1);
  sync_service()->OnGetExistingObjects(kPreferences, std::move(records),
                                       device1_timestamp, false);

  EXPECT_NE(brave_sync_prefs()->GetLastFetchTime(), base::Time());
  EXPECT_EQ(brave_sync_prefs()->GetLatestDeviceRecordTime(), device1_timestamp);

  // We have moved records into OnGetExistingObjects, so fill again
  records = std::make_unique<brave_sync::RecordsList>();
  records->push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "1", "device1"));
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  sync_service()->OnResolvedSyncRecords(kPreferences, std::move(records));

  // When there is only on device in chain, expect fetching sync records with
  // start_at==0
  ASSERT_EQ(brave_sync_prefs()->GetSyncDevices()->size(), 1u);
  EXPECT_CALL(*sync_client(), SendFetchSyncRecords(_, base::Time(), _))
      .Times(1);
  sync_service()->FetchDevices();

  // If number of devices becomes 2 or more we should next 70 sec use s3
  // with epmty start_at_time and then switch to sqs with non-empty
  // start_at_time
  records = std::make_unique<brave_sync::RecordsList>();
  records->push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "2", "device2"));
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  sync_service()->OnResolvedSyncRecords(kPreferences, std::move(records));

  // Chain has been created but 70 sec not yet passed
  ASSERT_EQ(brave_sync_prefs()->GetSyncDevices()->size(), 2u);
  EXPECT_CALL(*sync_client(), SendFetchSyncRecords(_, base::Time(), _))
      .Times(1);
  sync_service()->FetchDevices();

  // Less than 70 seconds have passed, we should fetch with empty start_at
  {
    auto time_override = OverrideForTimeDelta(base::TimeDelta::FromSeconds(65));
    EXPECT_CALL(*sync_client(), SendFetchSyncRecords(_, base::Time(), _))
        .Times(1);
    sync_service()->FetchDevices();
  }

  // More than 70 seconds have passed, we should fetch with non-empty start_at
  {
    auto time_override = OverrideForTimeDelta(base::TimeDelta::FromSeconds(75));
    EXPECT_CALL(*sync_client(),
                SendFetchSyncRecords(_, base::Time(device1_timestamp), _))
        .Times(1);
    sync_service()->FetchDevices();
  }
}
