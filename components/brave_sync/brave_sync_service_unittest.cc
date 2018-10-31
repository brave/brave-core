/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/common/brave_switches.h"
#include "brave/components/brave_sync/client/bookmark_change_processor.h"
#include "brave/components/brave_sync/client/brave_sync_client_impl.h"
#include "brave/components/brave_sync/client/client_ext_impl_data.h"
#include "brave/components/brave_sync/brave_sync_service_impl.h"
#include "brave/components/brave_sync/brave_sync_service_factory.h"
#include "brave/components/brave_sync/brave_sync_service_observer.h"
#include "brave/components/brave_sync/jslib_const.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/settings.h"
#include "brave/components/brave_sync/sync_devices.h"
#include "brave/components/brave_sync/test_util.h"
#include "brave/components/brave_sync/values_conv.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/test/test_bookmark_client.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/test_browser_thread_bundle.h"
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
// SendFetchSyncDevices     |
// SendResolveSyncRecords   |
// SendSyncRecords          |
// SendDeleteSyncUser       |
// SendDeleteSyncCategory   |
// SendGetBookmarksBaseOrder|
// NeedSyncWords            |
// NeedBytesFromSyncWords   |
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
// BackgroundSyncStarted     | +, BraveSyncServiceTest.BookmarkAddedImpl
// BackgroundSyncStopped     | +
// OnSyncDebug               | +
// OnSyncSetupError          | Need UI handler
// OnGetInitData             | +
// OnSaveInitData            | BraveSyncServiceTest.GetSeed
// OnSyncReady               | +
// OnGetExistingObjects      | +
// OnResolvedSyncRecords     | BraveSyncServiceTest.BookmarkAddedImpl
// OnDeletedSyncUser         | N/A
// OnDeleteSyncSiteSettings  | N/A
// OnSaveBookmarksBaseOrder  | +
// OnSyncWordsPrepared       | BraveSyncServiceTest.GetSyncWords
// OnResolvedHistorySites    | N/A
// OnResolvedPreferences     | BraveSyncServiceTest.OnDeleteDevice,
//                           | BraveSyncServiceTest.OnResetSync
// OnSyncPrefsChanged        | +

using testing::_;
using testing::AtLeast;
using namespace brave_sync;

class MockBraveSyncServiceObserver : public BraveSyncServiceObserver {
 public:
  MockBraveSyncServiceObserver() {}

  MOCK_METHOD1(OnSyncStateChanged, void(BraveSyncService*));
  MOCK_METHOD2(OnHaveSyncWords, void(BraveSyncService*, const std::string&));
  MOCK_METHOD2(OnLogMessage, void(BraveSyncService*, const std::string&));
};

class BraveSyncServiceTest : public testing::Test {
 public:
  BraveSyncServiceTest() {}
  ~BraveSyncServiceTest() override {}

 protected:
  void SetUp() override {
    EXPECT_TRUE(temp_dir_.CreateUniqueTempDir());
    // register the factory

    profile_ = CreateBraveSyncProfile(temp_dir_.GetPath());
    EXPECT_TRUE(profile_.get() != NULL);

    // TODO(bridiver) - this is temporary until some changes are made to
    // to bookmark_change_processor to allow `set_for_testing` like
    // BraveSyncClient
    BookmarkModelFactory::GetInstance()->SetTestingFactory(
       profile(), &BuildFakeBookmarkModelForTests);

    BraveSyncClientImpl::set_for_testing(
        new MockBraveSyncClient());

    sync_service_ = static_cast<BraveSyncServiceImpl*>(
        BraveSyncServiceFactory::GetInstance()->GetForProfile(profile()));

    sync_client_ =
        static_cast<MockBraveSyncClient*>(sync_service_->GetSyncClient());

    observer_.reset(new MockBraveSyncServiceObserver);
    sync_service_->AddObserver(observer_.get());
    EXPECT_TRUE(sync_service_ != NULL);
  }

  void TearDown() override {
    sync_service_->RemoveObserver(observer_.get());
    // this will also trigger a shutdown of the brave sync service
    profile_.reset();
  }

  void BookmarkAddedImpl();

  Profile* profile() { return profile_.get(); }
  BraveSyncServiceImpl* sync_service() { return sync_service_; }
  MockBraveSyncClient* sync_client() { return sync_client_; }
  MockBraveSyncServiceObserver* observer() { return observer_.get(); }

 private:
  // Need this as a very first member to run tests in UI thread
  // When this is set, class should not install any other MessageLoops, like
  // base::test::ScopedTaskEnvironment
  content::TestBrowserThreadBundle thread_bundle_;

  std::unique_ptr<Profile> profile_;
  BraveSyncServiceImpl* sync_service_;
  MockBraveSyncClient* sync_client_;
  std::unique_ptr<MockBraveSyncServiceObserver> observer_;

  base::ScopedTempDir temp_dir_;
};

TEST_F(BraveSyncServiceTest, SetSyncEnabled) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged);
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  EXPECT_FALSE(profile()->GetPrefs()->GetBoolean(
      brave_sync::prefs::kSyncEnabled));
  sync_service()->OnSetSyncEnabled(true);
  EXPECT_TRUE(profile()->GetPrefs()->GetBoolean(
      brave_sync::prefs::kSyncEnabled));
  EXPECT_FALSE(sync_service()->IsSyncInitialized());
  EXPECT_FALSE(sync_service()->IsSyncConfigured());
}

TEST_F(BraveSyncServiceTest, SetSyncDisabled) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  sync_service()->OnSetSyncEnabled(true);
  EXPECT_TRUE(profile()->GetPrefs()->GetBoolean(
      brave_sync::prefs::kSyncEnabled));

  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  sync_service()->OnSetSyncEnabled(false);
  EXPECT_FALSE(profile()->GetPrefs()->GetBoolean(
      brave_sync::prefs::kSyncEnabled));
  EXPECT_FALSE(sync_service()->IsSyncInitialized());
  EXPECT_FALSE(sync_service()->IsSyncConfigured());
}

TEST_F(BraveSyncServiceTest, IsSyncConfiguredOnNewProfile) {
  EXPECT_FALSE(sync_service()->IsSyncConfigured());
}

TEST_F(BraveSyncServiceTest, IsSyncInitializedOnNewProfile) {
  EXPECT_FALSE(sync_service()->IsSyncInitialized());
}

void BraveSyncServiceTest::BookmarkAddedImpl() {
  // BraveSyncService: real
  // BraveSyncClient: mock
  // Invoke BraveSyncService::BookmarkAdded
  // Expect BraveSyncClient::SendSyncRecords invoked
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(AtLeast(1));
  sync_service()->OnSetupSyncNewToSync("UnitTestBookmarkAdded");
  sync_service()->BackgroundSyncStarted(true/*startup*/);

  auto* bookmark_model = BookmarkModelFactory::GetForBrowserContext(profile());
  bookmarks::AddIfNotBookmarked(bookmark_model,
                                 GURL("https://a.com"),
                                 base::ASCIIToUTF16("A.com - title"));
  // Force service send bookmarks and fire the mock
  EXPECT_CALL(*sync_client(), SendSyncRecords(_,_)).Times(1);
  sync_service()->OnResolvedSyncRecords(brave_sync::jslib_const::kBookmarks,
    std::make_unique<RecordsList>());
}

TEST_F(BraveSyncServiceTest, BookmarkAdded) {
  BookmarkAddedImpl();
}

TEST_F(BraveSyncServiceTest, BookmarkDeleted) {
  BookmarkAddedImpl();
  auto* bookmark_model = BookmarkModelFactory::GetForBrowserContext(profile());

  // And just now can actually test delete
  std::vector<const bookmarks::BookmarkNode*> nodes;
  bookmarks::GetMostRecentlyAddedEntries(bookmark_model, 1, &nodes);
  ASSERT_EQ(nodes.size(), 1u);
  ASSERT_NE(nodes.at(0), nullptr);
  EXPECT_CALL(*sync_client(), SendSyncRecords(_,_)).Times(1); // TODO, AB: preciece with mock expect filter
  bookmark_model->Remove(nodes.at(0));
  // record->action = jslib::SyncRecord::Action::DELETE;
  // <= BookmarkNodeToSyncBookmark <= BookmarkChangeProcessor::SendUnsynced
  // <= BraveSyncServiceImpl::OnResolvedSyncRecords
  sync_service()->OnResolvedSyncRecords(brave_sync::jslib_const::kBookmarks,
    std::make_unique<RecordsList>());
}

TEST_F(BraveSyncServiceTest, OnSetupSyncHaveCode) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged);
  // Expecting sync state changed twice: for enabled state and for device name
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(2);
  sync_service()->OnSetupSyncHaveCode("word1 word2 word3", "test_device");
  EXPECT_TRUE(profile()->GetPrefs()->GetBoolean(
       brave_sync::prefs::kSyncEnabled));
}

TEST_F(BraveSyncServiceTest, OnSetupSyncNewToSync) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged);
  // Expecting sync state changed twice: for enabled state and for device name
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(2);
  sync_service()->OnSetupSyncNewToSync("test_device");
  EXPECT_TRUE(profile()->GetPrefs()->GetBoolean(
       brave_sync::prefs::kSyncEnabled));
}

TEST_F(BraveSyncServiceTest, GetSettingsAndDevices) {
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kEnableBraveSync);
  // The test absorbs OnSetupSyncNewToSync test
  auto callback1 = base::BindRepeating(
      [](std::unique_ptr<brave_sync::Settings> settings,
         std::unique_ptr<brave_sync::SyncDevices> devices) {
            EXPECT_TRUE(settings->this_device_name_.empty());
            EXPECT_FALSE(settings->sync_configured_);
            EXPECT_FALSE(settings->sync_this_device_);
            EXPECT_FALSE(settings->sync_bookmarks_);
            EXPECT_FALSE(settings->sync_settings_);
            EXPECT_FALSE(settings->sync_history_);
            EXPECT_EQ(devices->devices_.size(), 0u);
        }
  );
  sync_service()->GetSettingsAndDevices(callback1);
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged);
  // Expecting sync state changed twice: for enabled state and for device name
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(2);
  sync_service()->OnSetupSyncNewToSync("test_device");
  EXPECT_TRUE(profile()->GetPrefs()->GetBoolean(
       brave_sync::prefs::kSyncEnabled));
  auto callback2 = base::BindRepeating(
      [](std::unique_ptr<brave_sync::Settings> settings,
         std::unique_ptr<brave_sync::SyncDevices> devices) {
          // Other fields may be switched later
          EXPECT_EQ(settings->this_device_name_, "test_device");
          EXPECT_TRUE(settings->sync_this_device_);
      }
  );
  sync_service()->GetSettingsAndDevices(callback2);
}

TEST_F(BraveSyncServiceTest, GetSyncWords) {
  EXPECT_CALL(*sync_client(), NeedSyncWords);
  sync_service()->GetSyncWords();
  // The call should go to BraveSyncClient => BraveSyncEventRouter =>
  // background.js onNeedSyncWords => api::BraveSyncSyncWordsPreparedFunction =>
  // BraveSyncServiceImpl::OnSyncWordsPrepared
  // but as we have a mock instead of BraveSyncClient, emulate the response
  const std::string words = "word1 word2 word3";
  EXPECT_CALL(*observer(), OnHaveSyncWords(sync_service(), words)).Times(1);
  sync_service()->OnSyncWordsPrepared(words);
}

TEST_F(BraveSyncServiceTest, GetSeed) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged);
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(AtLeast(2));
  sync_service()->OnSetupSyncNewToSync("test_device");
  EXPECT_TRUE(profile()->GetPrefs()->GetBoolean(
       brave_sync::prefs::kSyncEnabled));

  // Service gets seed from client via BraveSyncServiceImpl::OnSaveInitData
  const auto binary_seed = Uint8Array(16, 77);

  sync_service()->OnSaveInitData(binary_seed, {0});
  std::string expected_seed = StrFromUint8Array(binary_seed);
  EXPECT_EQ(sync_service()->GetSeed(), expected_seed);
}

bool DevicesContains(SyncDevices* devices, const std::string& id,
    const std::string& name) {
  DCHECK(devices);
  for (const SyncDevice &device : devices->devices_) {
    if (device.device_id_ == id && device.name_ == name) {
      return true;
    }
  }
  return false;
}

MATCHER_P2(ContainsDeviceRecord, action, name,
    "contains device sync record with params") {
  for (const auto& record : arg) {
    if (record->has_device()) {
      const auto& device = record->GetDevice();
      if (record->action == action &&
          device.name == name) {
        return true;
      }
    }
  }
  return false;
}

TEST_F(BraveSyncServiceTest, OnDeleteDevice) {
  RecordsList records;
  records.push_back(SimpleDeviceRecord(
      jslib::SyncRecord::Action::CREATE,
      "1", "device1"));
  records.push_back(SimpleDeviceRecord(
      jslib::SyncRecord::Action::CREATE,
      "2", "device2"));
  records.push_back(SimpleDeviceRecord(
      jslib::SyncRecord::Action::CREATE,
      "3", "device3"));
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  sync_service()->OnResolvedPreferences(records);

  auto devices = sync_service()->sync_prefs_->GetSyncDevices();

  EXPECT_TRUE(DevicesContains(devices.get(), "1", "device1"));
  EXPECT_TRUE(DevicesContains(devices.get(), "2", "device2"));
  EXPECT_TRUE(DevicesContains(devices.get(), "3", "device3"));

  using brave_sync::jslib::SyncRecord;
  EXPECT_CALL(*sync_client(), SendSyncRecords("PREFERENCES",
      ContainsDeviceRecord(SyncRecord::Action::DELETE, "device3"))).Times(1);
  sync_service()->OnDeleteDevice("3");
  EXPECT_CALL(*sync_client(), SendSyncRecords("PREFERENCES",
      ContainsDeviceRecord(SyncRecord::Action::DELETE, "device2"))).Times(1);
  sync_service()->OnDeleteDevice("2");

  RecordsList resolved_records;
  auto resolved_record1 = SyncRecord::Clone(*records.at(2));
  resolved_record1->action = jslib::SyncRecord::Action::DELETE;
  resolved_records.push_back(std::move(resolved_record1));
  auto resolved_record2 = SyncRecord::Clone(*records.at(1));
  resolved_record2->action = jslib::SyncRecord::Action::DELETE;
  resolved_records.push_back(std::move(resolved_record2));
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  sync_service()->OnResolvedPreferences(resolved_records);

  auto devices_final = sync_service()->sync_prefs_->GetSyncDevices();
  EXPECT_TRUE(DevicesContains(devices_final.get(), "1", "device1"));
  EXPECT_FALSE(DevicesContains(devices_final.get(), "2", "device2"));
  EXPECT_FALSE(DevicesContains(devices_final.get(), "3", "device3"));
}

TEST_F(BraveSyncServiceTest, OnResetSync) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(AtLeast(2));
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(AtLeast(3));
  sync_service()->OnSetupSyncNewToSync("this_device");
  EXPECT_TRUE(profile()->GetPrefs()->GetBoolean(
       brave_sync::prefs::kSyncEnabled));

  RecordsList records;
  records.push_back(SimpleDeviceRecord(
      jslib::SyncRecord::Action::CREATE,
      "0", "this_device"));
  records.push_back(SimpleDeviceRecord(
      jslib::SyncRecord::Action::CREATE,
      "1", "device1"));

  sync_service()->OnResolvedPreferences(records);

  auto devices = sync_service()->sync_prefs_->GetSyncDevices();

  EXPECT_TRUE(DevicesContains(devices.get(), "0", "this_device"));
  EXPECT_TRUE(DevicesContains(devices.get(), "1", "device1"));

  sync_service()->OnResetSync();

  auto devices_final = sync_service()->sync_prefs_->GetSyncDevices();
  EXPECT_FALSE(DevicesContains(devices_final.get(), "0", "this_device"));
  EXPECT_FALSE(DevicesContains(devices_final.get(), "1", "device1"));

  EXPECT_TRUE(profile()->GetPrefs()->GetString(
      brave_sync::prefs::kSyncDeviceId).empty());
  EXPECT_TRUE(profile()->GetPrefs()->GetString(
      brave_sync::prefs::kSyncSeed).empty());
  EXPECT_TRUE(profile()->GetPrefs()->GetString(
      brave_sync::prefs::kSyncDeviceName).empty());
  EXPECT_FALSE(profile()->GetPrefs()->GetBoolean(
      brave_sync::prefs::kSyncEnabled));
  EXPECT_FALSE(profile()->GetPrefs()->GetBoolean(
      brave_sync::prefs::kSyncBookmarksEnabled));
  EXPECT_TRUE(profile()->GetPrefs()->GetString(
      brave_sync::prefs::kSyncBookmarksBaseOrder).empty());
  EXPECT_FALSE(profile()->GetPrefs()->GetBoolean(
      brave_sync::prefs::kSyncSiteSettingsEnabled));
  EXPECT_FALSE(profile()->GetPrefs()->GetBoolean(
      brave_sync::prefs::kSyncHistoryEnabled));
  EXPECT_TRUE(profile()->GetPrefs()->GetTime(
      brave_sync::prefs::kSyncLatestRecordTime).is_null());
  EXPECT_TRUE(profile()->GetPrefs()->GetTime(
      brave_sync::prefs::kSyncLastFetchTime).is_null());
  EXPECT_TRUE(profile()->GetPrefs()->GetString(
      brave_sync::prefs::kSyncDeviceList).empty());
  EXPECT_EQ(profile()->GetPrefs()->GetString(
      brave_sync::prefs::kSyncApiVersion), "0");

  EXPECT_FALSE(sync_service()->IsSyncInitialized());
  EXPECT_FALSE(sync_service()->IsSyncConfigured());
}

TEST_F(BraveSyncServiceTest, OnSetSyncBookmarks) {
  EXPECT_FALSE(profile()->GetPrefs()->GetBoolean(
       brave_sync::prefs::kSyncBookmarksEnabled));
  EXPECT_CALL(*observer(), OnSyncStateChanged).Times(1);
  sync_service()->OnSetSyncBookmarks(true);
  EXPECT_TRUE(profile()->GetPrefs()->GetBoolean(
       brave_sync::prefs::kSyncBookmarksEnabled));
  EXPECT_CALL(*observer(), OnSyncStateChanged).Times(1);
  sync_service()->OnSetSyncBookmarks(false);
  EXPECT_FALSE(profile()->GetPrefs()->GetBoolean(
      brave_sync::prefs::kSyncBookmarksEnabled));
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
  sync_service()->OnSaveBookmarksBaseOrder("1.1.");
  EXPECT_EQ(profile()->GetPrefs()->GetString(
       brave_sync::prefs::kSyncBookmarksBaseOrder), "1.1.");
}

TEST_F(BraveSyncServiceTest, OnSyncPrefsChanged) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);
  EXPECT_CALL(*observer(), OnSyncStateChanged);
  sync_service()->OnSyncPrefsChanged(brave_sync::prefs::kSyncEnabled);
}

TEST_F(BraveSyncServiceTest, OnSyncDebug) {
  EXPECT_CALL(*observer(), OnLogMessage(sync_service(), "message")).Times(1);
  sync_service()->OnSyncDebug("message");
}

TEST_F(BraveSyncServiceTest, OnSyncReadyAlreadyWithSync) {
  EXPECT_FALSE(sync_service()->IsSyncInitialized());
  profile()->GetPrefs()->SetString(
                           brave_sync::prefs::kSyncBookmarksBaseOrder, "1.1.");
  // OnSyncPrefsChanged => OnSyncStateChanged for kSyncSiteSettingsEnabled
  EXPECT_CALL(*observer(), OnSyncStateChanged);
  profile()->GetPrefs()->SetBoolean(
                            brave_sync::prefs::kSyncSiteSettingsEnabled, true);
  profile()->GetPrefs()->SetTime(
                     brave_sync::prefs::kSyncLastFetchTime, base::Time::Now());
  EXPECT_CALL(*sync_client(), SendFetchSyncRecords).Times(1);
  EXPECT_CALL(*sync_client(), SendFetchSyncDevices).Times(1);
  sync_service()->OnSyncReady();
  EXPECT_TRUE(sync_service()->IsSyncInitialized());
}

TEST_F(BraveSyncServiceTest, OnSyncReadyNewToSync) {
  EXPECT_CALL(*observer(), OnSyncStateChanged);
  profile()->GetPrefs()->SetBoolean(
                            brave_sync::prefs::kSyncSiteSettingsEnabled, true);
  EXPECT_CALL(*sync_client(), SendGetBookmarksBaseOrder).Times(1);
  sync_service()->OnSyncReady();
}

TEST_F(BraveSyncServiceTest, OnGetExistingObjects) {
  EXPECT_CALL(*sync_client(), SendResolveSyncRecords).Times(1);

  auto records = std::make_unique<RecordsList>();
  sync_service()->OnGetExistingObjects(jslib_const::kBookmarks,
      std::move(records),
      base::Time(),
      false);
}

TEST_F(BraveSyncServiceTest, BackgroundSyncStarted) {
  sync_service()->BackgroundSyncStarted(false);
  EXPECT_TRUE(sync_service()->timer_->IsRunning());
}

TEST_F(BraveSyncServiceTest, BackgroundSyncStopped) {
  sync_service()->BackgroundSyncStopped(false);
  EXPECT_FALSE(sync_service()->timer_->IsRunning());
}
