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
using brave_sync::RecordsListPtr;
using brave_sync::SimpleBookmarkSyncRecord;
using brave_sync::SimpleDeviceRecord;
using brave_sync::SimpleFolderSyncRecord;
using brave_sync::jslib_const::kBookmarks;
using brave_sync::jslib_const::kPreferences;
using brave_sync::jslib::SyncRecord;
using brave_sync::tools::AsMutable;
using brave_sync::tools::GenerateObjectId;
using brave_sync::tools::GenerateObjectIdForOtherNode;
using brave_sync::tools::kOtherNodeName;
using brave_sync::tools::kOtherNodeOrder;
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
                     const std::string& id_v2,
                     const std::string& name) {
  DCHECK(devices);
  for (const auto& device : devices->devices_) {
    if (device.device_id_ == id && device.name_ == name &&
        device.device_id_v2_ == id_v2) {
      return true;
    }
  }
  return false;
}

MATCHER_P3(ContainsDeviceRecord,
           action,
           name,
           device_id_v2,
           "contains device sync record with params") {
  for (const auto& record : arg) {
    if (record->has_device()) {
      const auto& device = record->GetDevice();
      if (record->action == action && device.name == name &&
          device.deviceIdV2 == device_id_v2) {
        return true;
      }
    }
  }
  return false;
}

MATCHER_P(MatchBookmarksRecords,
          records,
          "Match bookmark sync records") {
  if (arg.size() != records->size())
    return false;
  for (size_t i = 0; i < arg.size(); ++i) {
    if (!arg.at(i)->has_bookmark() || !records->at(i)->has_bookmark())
      return false;
    if (!arg.at(i)->Matches(*records->at(i).get()))
      return false;
  }
  return true;
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

    SetBookmarkModelFactory();

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

  virtual void SetBookmarkModelFactory() {
    // TODO(bridiver) - this is temporary until some changes are made to
    // to bookmark_change_processor to allow `set_for_testing` like
    // BraveSyncClient
    BookmarkModelFactory::GetInstance()->SetTestingFactory(
        profile(),
        base::BindRepeating(&brave_sync::BuildFakeBookmarkModelForTests));
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

  // Common part of OnResetSync and OnResetSyncWhenOffline tests
  void VerifyResetDone();

 private:
  // Need this as a very first member to run tests in UI thread
  // When this is set, class should not install any other MessageLoops, like
  // base::test::ScopedTaskEnvironment
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<Profile> profile_;
  BraveProfileSyncServiceImpl* sync_service_;
  MockBraveSyncClient* sync_client_;
  BookmarkModel* model_;  // Not owns
  std::unique_ptr<syncer::SyncPrefs> sync_prefs_;
  std::unique_ptr<MockBraveSyncServiceObserver> observer_;

  base::ScopedTempDir temp_dir_;
};

class BraveSyncServiceTestDelayedLoadModel : public BraveSyncServiceTest {
 public:
  void SetBookmarkModelFactory() override {
    BookmarkModelFactory::GetInstance()->SetTestingFactory(
        profile(),
        base::BindRepeating(&BraveSyncServiceTestDelayedLoadModel::BuildModel,
                            base::Unretained(this)));
  }
  void ModelDoneLoading() {
    model()->DoneLoading(std::move(bookmark_load_details_));
  }

 private:
  std::unique_ptr<KeyedService> BuildModel(content::BrowserContext* context) {
    using bookmarks::BookmarkClient;
    using bookmarks::BookmarkLoadDetails;
    using bookmarks::BookmarkModel;
    using bookmarks::TestBookmarkClient;
    std::unique_ptr<TestBookmarkClient> client(new TestBookmarkClient());
    BookmarkClient* client_ptr = client.get();
    std::unique_ptr<BookmarkModel> bookmark_model(
        new BookmarkModel(std::move(client)));
    bookmark_load_details_ = std::make_unique<BookmarkLoadDetails>(client_ptr);
    bookmark_load_details_->LoadManagedNode();
    bookmark_load_details_->CreateUrlIndex();
    // By intent don't call BookmarkModel::DoneLoading
    return bookmark_model;
  }
  std::unique_ptr<bookmarks::BookmarkLoadDetails> bookmark_load_details_;
};

TEST_F(BraveSyncServiceTest, SetSyncEnabled) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged);
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  EXPECT_FALSE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));
  sync_service()->OnSetSyncEnabled(true);
  EXPECT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));
  EXPECT_FALSE(sync_service()->brave_sync_ready_);
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
  EXPECT_FALSE(sync_service()->brave_sync_ready_);
}

TEST_F(BraveSyncServiceTest, IsSyncReadyOnNewProfile) {
  EXPECT_FALSE(sync_service()->brave_sync_ready_);
}

const char kTestWords1[] =
    "absurd avoid scissors anxiety gather lottery category door "
    "army half long cage bachelor another expect people blade "
    "school educate curtain scrub monitor lady beyond";

TEST_F(BraveSyncServiceTest, OnSetupSyncHaveCode) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged);
  // Expecting sync state changed twice: for enabled state and for device name
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(2);
  sync_service()->OnSetupSyncHaveCode(kTestWords1, "test_device");
  EXPECT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));
}

TEST_F(BraveSyncServiceTest, OnSetupSyncHaveCodeIncorrectCode) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(0);
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(0);
  EXPECT_CALL(*observer(),
              OnSyncSetupError(sync_service(), "ERR_SYNC_WRONG_WORDS"))
      .Times(1);
  sync_service()->OnSetupSyncHaveCode("wrong code", "test_device");
  EXPECT_FALSE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));
}

TEST_F(BraveSyncServiceTest, OnSetupSyncHaveCodeEmptyCode) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(0);
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(0);
  EXPECT_CALL(*observer(),
              OnSyncSetupError(sync_service(), "ERR_SYNC_WRONG_WORDS"))
      .Times(1);
  sync_service()->OnSetupSyncHaveCode("", "test_device");
  EXPECT_FALSE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));
}

TEST_F(BraveSyncServiceTest, OnSetupSyncHaveCodeEmptyDeviceName) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged);
  // Expecting sync state changed twice: for enabled state and for device name
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(2);
  sync_service()->OnSetupSyncHaveCode(kTestWords1, "");
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

  const std::string device_id_v2 = "beef12";
  sync_service()->OnSaveInitData(binary_seed, {0}, device_id_v2);
  std::string expected_seed = brave_sync::StrFromUint8Array(binary_seed);
  EXPECT_EQ(sync_service()->GetSeed(), expected_seed);
  EXPECT_EQ(brave_sync_prefs()->GetThisDeviceIdV2(), device_id_v2);
}

TEST_F(BraveSyncServiceTest, OnDeleteDevice) {
  RecordsList records;
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "", "1", "beef01",
                         "device1"));
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "", "2", "beef02",
                         "device2"));
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "", "3", "beef03",
                         "device3"));
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  sync_service()->OnResolvedPreferences(records);

  brave_sync_prefs()->SetThisDeviceId("1");
  brave_sync_prefs()->SetThisDeviceIdV2("beef01");
  auto devices = brave_sync_prefs()->GetSyncDevices();

  EXPECT_TRUE(DevicesContains(devices.get(), "1", "beef01", "device1"));
  EXPECT_TRUE(DevicesContains(devices.get(), "2", "beef02", "device2"));
  EXPECT_TRUE(DevicesContains(devices.get(), "3", "beef03", "device3"));

  EXPECT_CALL(*sync_client(),
              SendSyncRecords(kPreferences,
                              ContainsDeviceRecord(SyncRecord::Action::A_DELETE,
                                                   "device3", "beef03")))
      .Times(1);
  EXPECT_CALL(*sync_client(), SendFetchSyncRecords(_, _, _));
  sync_service()->OnDeleteDevice("beef03");

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
  EXPECT_TRUE(DevicesContains(devices_final.get(), "1", "beef01", "device1"));
  EXPECT_TRUE(DevicesContains(devices_final.get(), "2", "beef02", "device2"));
  EXPECT_FALSE(DevicesContains(devices_final.get(), "3", "beef03", "device3"));
}

TEST_F(BraveSyncServiceTest, OnDeleteDeviceWhenOneDevice) {
  const std::string object_id_1 = "0, 0, 0, 0, 0, 0, 0, 0, "
                                  "0, 0, 0, 0, 0, 0, 0, 1";
  const std::string object_id_2 = "0, 0, 0, 0, 0, 0, 0, 0, "
                                  "0, 0, 0, 0, 0, 0, 0, 2";
  brave_sync_prefs()->SetThisDeviceId("1");
  brave_sync_prefs()->SetThisDeviceObjectId(object_id_1);
  brave_sync_prefs()->SetThisDeviceIdV2("beef01");
  RecordsList records;
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, object_id_1, "1",
                         "beef01", "device1"));
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, object_id_2, "2",
                         "beef02", "device2"));
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  sync_service()->OnResolvedPreferences(records);

  auto devices = brave_sync_prefs()->GetSyncDevices();

  EXPECT_TRUE(DevicesContains(devices.get(), "1", "beef01", "device1"));
  EXPECT_TRUE(DevicesContains(devices.get(), "2", "beef02", "device2"));

  EXPECT_CALL(*sync_client(), SendSyncRecords).Times(1);
  EXPECT_CALL(*sync_client(), SendFetchSyncRecords(_, _, _));
  sync_service()->OnDeleteDevice("beef02");

  RecordsList resolved_records;
  auto resolved_record = SyncRecord::Clone(*records.at(1));
  resolved_record->action = SyncRecord::Action::A_DELETE;
  resolved_records.push_back(std::move(resolved_record));
  // Expecting to be called one time to set the new devices list
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);

  EXPECT_CALL(*sync_client(), SendSyncRecords).Times(0);

  // Still enabled
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(0);

  sync_service()->OnResolvedPreferences(resolved_records);

  auto devices_semi_final = brave_sync_prefs()->GetSyncDevices();
  EXPECT_FALSE(DevicesContains(devices_semi_final.get(), "2", "beef02",
                               "device2"));
  EXPECT_TRUE(DevicesContains(devices_semi_final.get(), "1", "beef01",
                              "device1"));
}

TEST_F(BraveSyncServiceTest, OnDeleteDeviceWhenSelfDeleted) {
  const std::string object_id_1 = "0, 0, 0, 0, 0, 0, 0, 0, "
                                  "0, 0, 0, 0, 0, 0, 0, 1";
  const std::string object_id_2 = "0, 0, 0, 0, 0, 0, 0, 0, "
                                  "0, 0, 0, 0, 0, 0, 0, 2";
  brave_sync_prefs()->SetThisDeviceId("1");
  brave_sync_prefs()->SetThisDeviceObjectId(object_id_1);
  brave_sync_prefs()->SetThisDeviceIdV2("beef01");
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  brave_sync_prefs()->SetSyncEnabled(true);
  RecordsList records;
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, object_id_1, "1",
                         "beef01", "device1"));
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, object_id_2, "2",
                         "beef02", "device2"));
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  sync_service()->OnResolvedPreferences(records);

  auto devices = brave_sync_prefs()->GetSyncDevices();

  EXPECT_TRUE(DevicesContains(devices.get(), "1", "beef01", "device1"));
  EXPECT_TRUE(DevicesContains(devices.get(), "2", "beef02", "device2"));

  EXPECT_CALL(*sync_client(),
              SendSyncRecords(kPreferences,
                              ContainsDeviceRecord(SyncRecord::Action::A_DELETE,
                                                   "device1", "beef01")))
      .Times(1);
  EXPECT_CALL(*sync_client(), SendFetchSyncRecords(_, _, _));
  sync_service()->OnDeleteDevice("beef01");

  // Enabled->Disabled
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);
  RecordsList resolved_records;
  auto resolved_record = SyncRecord::Clone(*records.at(0));
  resolved_record->action = SyncRecord::Action::A_DELETE;
  resolved_records.push_back(std::move(resolved_record));
  // If you have to modify .Times(3) to another value, double re-check
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(3);
  sync_service()->OnResolvedPreferences(resolved_records);

  EXPECT_FALSE(brave_sync_prefs()->GetSyncEnabled());

  auto devices_final = brave_sync_prefs()->GetSyncDevices();
  EXPECT_FALSE(DevicesContains(devices_final.get(), "1", "beef01", "device1"));
  EXPECT_FALSE(DevicesContains(devices_final.get(), "2", "beef02", "device2"));
}

void BraveSyncServiceTest::VerifyResetDone() {
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

  EXPECT_FALSE(sync_service()->brave_sync_ready_);

  EXPECT_FALSE(sync_prefs()->IsSyncRequested());
  EXPECT_FALSE(
      profile()->GetPrefs()->GetBoolean(syncer::prefs::kSyncBookmarks));
}

TEST_F(BraveSyncServiceTest, OnResetSync) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(AtLeast(1));
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service()))
      .Times(AtLeast(3));
  sync_service()->OnSetupSyncNewToSync("this_device");
  EXPECT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));
  const std::string object_id_0 = "0, 0, 0, 0, 0, 0, 0, 0, "
                                  "0, 0, 0, 0, 0, 0, 0, 0";
  const std::string object_id_1 = "0, 0, 0, 0, 0, 0, 0, 0, "
                                  "0, 0, 0, 0, 0, 0, 0, 1";
  brave_sync_prefs()->SetThisDeviceId("0");
  brave_sync_prefs()->SetThisDeviceObjectId(object_id_0);
  brave_sync_prefs()->SetThisDeviceIdV2("beef00");

  RecordsList records;
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, object_id_0, "0",
                         "beef00", "this_device"));
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, object_id_1, "1",
                         "beef01", "device1"));

  sync_service()->OnResolvedPreferences(records);

  auto devices = brave_sync_prefs()->GetSyncDevices();

  EXPECT_TRUE(DevicesContains(devices.get(), "0", "beef00", "this_device"));
  EXPECT_TRUE(DevicesContains(devices.get(), "1", "beef01", "device1"));

  EXPECT_CALL(*sync_client(),
              SendSyncRecords(kPreferences,
                              ContainsDeviceRecord(SyncRecord::Action::A_DELETE,
                                                   "this_device", "beef00")))
      .Times(1);

  EXPECT_CALL(*sync_client(), SendFetchSyncRecords);
  sync_service()->OnResetSync();
  RecordsList resolved_records;
  auto resolved_record = SyncRecord::Clone(*records.at(0));
  resolved_record->action = SyncRecord::Action::A_DELETE;
  resolved_records.push_back(std::move(resolved_record));

  sync_service()->OnResolvedPreferences(resolved_records);

  auto devices_final = brave_sync_prefs()->GetSyncDevices();
  EXPECT_FALSE(DevicesContains(devices_final.get(), "0", "beef00",
                               "this_device"));
  EXPECT_FALSE(DevicesContains(devices_final.get(), "1", "beef01",
                               "device1"));

  VerifyResetDone();
}

TEST_F(BraveSyncServiceTest, OnResetSyncWhenOffline) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(AtLeast(1));
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service()))
      .Times(AtLeast(3));
  sync_service()->OnSetupSyncNewToSync("this_device");
  EXPECT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));
  brave_sync_prefs()->SetThisDeviceId("0");
  brave_sync_prefs()->SetThisDeviceIdV2("beef00");

  RecordsList records;
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "", "0", "beef00",
                         "this_device"));
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "", "1", "beef01",
                         "device1"));

  sync_service()->OnResolvedPreferences(records);

  auto devices = brave_sync_prefs()->GetSyncDevices();

  EXPECT_TRUE(DevicesContains(devices.get(), "0", "beef00", "this_device"));
  EXPECT_TRUE(DevicesContains(devices.get(), "1", "beef01", "device1"));

  EXPECT_CALL(*sync_client(),
              SendSyncRecords(kPreferences,
                              ContainsDeviceRecord(SyncRecord::Action::A_DELETE,
                                                   "this_device", "beef00")))
      .Times(1);

  EXPECT_CALL(*sync_client(), SendFetchSyncRecords);
  EXPECT_FALSE(sync_service()->pending_self_reset_);
  sync_service()->OnResetSync();
  EXPECT_TRUE(sync_service()->pending_self_reset_);

  auto recordsSent = std::make_unique<RecordsList>();
  sync_service()->OnRecordsSent(kPreferences, std::move(recordsSent));
  EXPECT_FALSE(sync_service()->pending_self_reset_);

  auto devices_final = brave_sync_prefs()->GetSyncDevices();
  EXPECT_FALSE(DevicesContains(devices_final.get(), "0", "beef00",
                               "this_device"));
  EXPECT_FALSE(DevicesContains(devices_final.get(), "1", "beef01",
                               "device1"));

  VerifyResetDone();
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
  EXPECT_EQ(order, kOtherNodeOrder);
  EXPECT_EQ(brave_sync_prefs()->GetMigratedBookmarksVersion(), 2);
}

TEST_F(BraveSyncServiceTest, OnBraveSyncPrefsChanged) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);
  EXPECT_CALL(*observer(), OnSyncStateChanged);
  sync_service()->OnBraveSyncPrefsChanged(brave_sync::prefs::kSyncEnabled);
}

void OnGetRecordsStub(std::unique_ptr<RecordsList> records) {}

TEST_F(BraveSyncServiceTest, SetThisDeviceCreatedTime) {
  EXPECT_TRUE(brave_sync::tools::IsTimeEmpty(
      sync_service()->this_device_created_time_));

  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);
  EXPECT_CALL(*observer(), OnSyncStateChanged);
  brave_sync_prefs()->SetSyncEnabled(true);
  brave_sync_prefs()->SetThisDeviceId("1");
  brave_sync_prefs()->SetThisDeviceIdV2("beef01");

  brave_sync::GetRecordsCallback on_get_records =
      base::BindOnce(&OnGetRecordsStub);
  base::WaitableEvent we;
  EXPECT_CALL(*sync_client(), SendSyncRecords(kPreferences, _));
  EXPECT_CALL(*sync_client(), SendFetchSyncRecords);
  sync_service()->OnPollSyncCycle(std::move(on_get_records), &we);

  EXPECT_FALSE(brave_sync::tools::IsTimeEmpty(
      sync_service()->this_device_created_time_));
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

TEST_F(BraveSyncServiceTestDelayedLoadModel, OnSyncReadyModelNotYetLoaded) {
  EXPECT_NE(sync_service()->model_, nullptr);
  EXPECT_FALSE(model()->loaded());
  profile()->GetPrefs()->SetString(brave_sync::prefs::kSyncBookmarksBaseOrder,
                                   "1.1.");

  EXPECT_FALSE(sync_service()->is_model_loaded_observer_set_);
  sync_service()->OnSyncReady();
  EXPECT_TRUE(sync_service()->is_model_loaded_observer_set_);
  EXPECT_CALL(*observer(), OnSyncStateChanged);
  ModelDoneLoading();

  // This pref is enabled in observer, check wether the observer job done
  EXPECT_TRUE(profile()->GetPrefs()->GetBoolean(syncer::prefs::kSyncBookmarks));
}

TEST_F(BraveSyncServiceTest, OnGetExistingObjects) {
  EXPECT_CALL(*sync_client(), SendResolveSyncRecords).Times(1);

  auto records = std::make_unique<RecordsList>();
  sync_service()->OnGetExistingObjects(kBookmarks,
                                       std::move(records), base::Time(), false);
}

TEST_F(BraveSyncServiceTest, GetPreferredDataTypes) {
  // Make sure GetPreferredDataTypes contains DEVICE_INFO which is needed for
  // brave device record polling
  EXPECT_TRUE(sync_service()->GetPreferredDataTypes().Has(syncer::DEVICE_INFO));
}

TEST_F(BraveSyncServiceTest, GetDisableReasons) {
  sync_prefs()->SetManagedForTest(true);
  EXPECT_TRUE(sync_service()->GetDisableReasons().Has(
                  syncer::SyncService::DISABLE_REASON_ENTERPRISE_POLICY) &&
              sync_service()->GetDisableReasons().Has(
                  syncer::SyncService::DISABLE_REASON_USER_CHOICE));
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  sync_service()->OnSetSyncEnabled(true);
  EXPECT_TRUE(sync_service()->GetDisableReasons().Has(
                  syncer::SyncService::DISABLE_REASON_ENTERPRISE_POLICY) &&
              sync_service()->GetDisableReasons().Has(
                  syncer::SyncService::DISABLE_REASON_USER_CHOICE));
  brave_sync_prefs()->SetMigratedBookmarksVersion(2);
  EXPECT_TRUE(sync_service()->GetDisableReasons().Empty());
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  sync_service()->OnSetSyncEnabled(false);
  EXPECT_TRUE(sync_service()->HasDisableReason(
      syncer::SyncService::DISABLE_REASON_ENTERPRISE_POLICY));
}

TEST_F(BraveSyncServiceTest, OnSetupSyncHaveCode_Reset_SetupAgain) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);
  // Expecting sync state changed twice: for enabled state and for device name
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(2);
  sync_service()->OnSetupSyncHaveCode(kTestWords1, "test_device");
  EXPECT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));

  brave_sync::Uint8Array seed = {1, 2,  3,  4,  5,  6,  7,  8,
                                 9, 10, 11, 12, 13, 14, 15, 16};
  brave_sync::Uint8Array device_id = {0};
  const std::string device_id_v2 = "beef00";
  sync_service()->OnSaveInitData(seed, device_id, device_id_v2);

  const std::string object_id = "0, 0, 0, 0, 0, 0, 0, 0, "
                                "0, 0, 0, 0, 0, 0, 0, 0";
  brave_sync_prefs()->SetThisDeviceObjectId(object_id);
  RecordsList records;
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, object_id, "0", "beef00",
                         "this_device"));
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  sync_service()->OnResolvedPreferences(records);

  auto devices = brave_sync_prefs()->GetSyncDevices();

  ASSERT_EQ(devices->size(), 1u);
  EXPECT_TRUE(DevicesContains(devices.get(), "0", "beef00", "this_device"));
  EXPECT_CALL(*sync_client(),
              SendSyncRecords(kPreferences,
                              ContainsDeviceRecord(SyncRecord::Action::A_DELETE,
                                                   "this_device", "beef00")))
      .Times(1);
  EXPECT_CALL(*sync_client(), SendFetchSyncRecords);
  sync_service()->OnResetSync();

  // Actual kSyncEnabled will go to false after receiving confirmation of
  // this_device DELETE
  EXPECT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));

  // Emulating resolved with DELETE
  records.at(0)->action = SyncRecord::Action::A_DELETE;
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);
  // Sync state is changed 4 times:
  // 1) we do save updated devices list
  // 2,3,4) we do ResetSyncInternal() which clears this device name,
  //        sync enabled and devices list
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(4);
  sync_service()->OnResolvedPreferences(records);

  EXPECT_FALSE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));

  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(2);
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);
  sync_service()->OnSetupSyncHaveCode(
      "anxiety path library anxiety gather lottery category door army half "
      "long cage bachelor another expect people blade school "
      "educate curtain scrub monitor lady arctic",
      "test_device");

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

  EXPECT_CALL(*sync_client(), SendSyncRecords(kBookmarks, _)).Times(1);
  sync_service()->SendSyncRecords(kBookmarks, std::move(records));

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
    auto time_override = OverrideForTimeDelta(base::TimeDelta::FromMinutes(i));
    bool is_send_expected = contains(should_sent_at, i);
    int expect_call_times = is_send_expected ? 1 : 0;
    EXPECT_CALL(*sync_client(), SendSyncRecords(kBookmarks, _))
        .Times(expect_call_times);
    sync_service()->ResendSyncRecords(kBookmarks);

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
  std::unique_ptr<RecordsList> sent_records = std::make_unique<RecordsList>();
  sent_records->push_back(SimpleBookmarkSyncRecord(
      SyncRecord::Action::A_CREATE, record_a_object_id, "https://a.com/",
      "A.com", "1.1.1.1", "", brave_sync_prefs()->GetThisDeviceId()));
  auto timestamp_resolve = base::Time::Now();
  sent_records->at(0)->syncTimestamp = timestamp_resolve;
  sync_service()->OnRecordsSent(kBookmarks, std::move(sent_records));

  EXPECT_EQ(brave_sync_prefs()->GetRecordsToResend().size(), 0u);
  EXPECT_EQ(brave_sync_prefs()->GetRecordToResendMeta(record_a_object_id),
            nullptr);
}

TEST_F(BraveSyncServiceTest, GetDevicesWithFetchSyncRecords) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);
  EXPECT_CALL(*observer(), OnSyncStateChanged);
  brave_sync_prefs()->SetSyncEnabled(true);
  brave_sync_prefs()->SetThisDeviceId("1");
  brave_sync_prefs()->SetThisDeviceIdV2("beef01");

  // Expecting SendFetchSyncRecords will be invoked
  // after sync_service()->OnPollSyncCycle
  EXPECT_EQ(brave_sync_prefs()->GetLastFetchTime(), base::Time());
  EXPECT_EQ(brave_sync_prefs()->GetLatestDeviceRecordTime(), base::Time());

  using brave_sync::tools::IsTimeEmpty;
  EXPECT_TRUE(IsTimeEmpty(sync_service()->this_device_created_time_));

  brave_sync::GetRecordsCallback on_get_records =
      base::BindOnce(&OnGetRecordsStub);
  base::WaitableEvent we;
  EXPECT_CALL(*sync_client(), SendSyncRecords("PREFERENCES", _));
  EXPECT_CALL(*sync_client(), SendFetchSyncRecords(_, base::Time(), _));
  sync_service()->OnPollSyncCycle(std::move(on_get_records), &we);
  EXPECT_FALSE(IsTimeEmpty(sync_service()->this_device_created_time_));

  // Emulate we received this device
  auto records = std::make_unique<brave_sync::RecordsList>();
  records->push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "", "1", "beef01",
                         "device1"));
  auto device1_timestamp = records->back()->syncTimestamp;
  EXPECT_CALL(*sync_client(), SendResolveSyncRecords(kPreferences, _)).Times(1);
  sync_service()->OnGetExistingObjects(kPreferences, std::move(records),
                                       device1_timestamp, false);

  EXPECT_NE(brave_sync_prefs()->GetLastFetchTime(), base::Time());
  EXPECT_EQ(brave_sync_prefs()->GetLatestDeviceRecordTime(), device1_timestamp);

  // We have moved records into OnGetExistingObjects, so fill again
  records = std::make_unique<brave_sync::RecordsList>();
  records->push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "", "1", "beef01",
                         "device1"));
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(1);
  sync_service()->OnResolvedSyncRecords(kPreferences, std::move(records));

  EXPECT_FALSE(IsTimeEmpty(sync_service()->this_device_created_time_));

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

TEST_F(BraveSyncServiceTest, SendCompact) {
  EXPECT_EQ(brave_sync_prefs()->GetLastCompactTimeBookmarks(), base::Time());
  EXPECT_CALL(*sync_client(), SendCompact(kBookmarks)).Times(1);
  EXPECT_CALL(*sync_client(), SendFetchSyncRecords).Times(1);
  sync_service()->FetchSyncRecords(true, false, true, 1000);
  // timestamp is not writtend until we get CompactedSyncCategory
  EXPECT_CALL(*sync_client(), SendCompact(kBookmarks)).Times(1);
  EXPECT_CALL(*sync_client(), SendFetchSyncRecords).Times(1);
  sync_service()->FetchSyncRecords(true, false, true, 1000);
  sync_service()->OnCompactComplete(kBookmarks);
  EXPECT_CALL(*sync_client(), SendCompact(kBookmarks)).Times(0);
  EXPECT_CALL(*sync_client(), SendFetchSyncRecords).Times(1);
  sync_service()->FetchSyncRecords(true, false, true, 1000);
  {
    auto time_override = OverrideForTimeDelta(base::TimeDelta::FromDays(
        sync_service()->GetCompactPeriodInDaysForTests()));
    EXPECT_CALL(*sync_client(), SendCompact(kBookmarks)).Times(1);
    EXPECT_CALL(*sync_client(), SendFetchSyncRecords).Times(1);
    sync_service()->FetchSyncRecords(true, false, true, 1000);
    sync_service()->OnCompactComplete(kBookmarks);
  }
  EXPECT_CALL(*sync_client(), SendCompact(kBookmarks)).Times(0);
  EXPECT_CALL(*sync_client(), SendFetchSyncRecords).Times(1);
  sync_service()->FetchSyncRecords(true, false, true, 1000);
}

TEST_F(BraveSyncServiceTest, MigratePrevSeed) {
  profile()->GetPrefs()->SetString(brave_sync::prefs::kSyncPrevSeed, "1,2,3");
  MigrateBraveSyncPrefs(profile()->GetPrefs());
  EXPECT_EQ(profile()->GetPrefs()->GetString(brave_sync::prefs::kSyncPrevSeed),
            "");
}

TEST_F(BraveSyncServiceTest, InitialFetchesStartWithZero) {
  EXPECT_CALL(*sync_client(), SendCompact(kBookmarks)).Times(1);
  EXPECT_CALL(*sync_client(), SendFetchSyncRecords(_, base::Time(), _))
      .Times(1);
  sync_service()->FetchSyncRecords(true, false, false, 1000);

  const auto latest_bookmark_record_time = base::Time::Now();
  brave_sync_prefs()->SetLatestRecordTime(latest_bookmark_record_time);
  sync_service()->this_device_created_time_ = base::Time::Now();

  EXPECT_CALL(*sync_client(), SendCompact(kBookmarks)).Times(1);
  EXPECT_CALL(*sync_client(), SendFetchSyncRecords(_, base::Time(), _))
      .Times(1);
  sync_service()->FetchSyncRecords(true, false, false, 1000);
  sync_service()->OnCompactComplete(kBookmarks);

  // Prior 70 sec we should use null start_at and after we should use
  // the latest record time
  {
    auto time_override = OverrideForTimeDelta(base::TimeDelta::FromSeconds(71));
    EXPECT_CALL(*sync_client(),
                SendFetchSyncRecords(_, latest_bookmark_record_time, _))
        .Times(1);
    sync_service()->FetchSyncRecords(true, false, false, 1000);
  }
}

TEST_F(BraveSyncServiceTest, DeviceIdV2Migration) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged);
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service()))
      .Times(AtLeast(2));
  sync_service()->OnSetupSyncNewToSync("device0");
  EXPECT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));

  // Service gets seed from client via BraveSyncServiceImpl::OnSaveInitData
  const auto binary_seed = brave_sync::Uint8Array(16, 77);

  // emulate device without device id v2
  sync_service()->OnSaveInitData(binary_seed, {0}, "");
  std::string expected_seed = brave_sync::StrFromUint8Array(binary_seed);
  EXPECT_EQ(sync_service()->GetSeed(), expected_seed);
  EXPECT_EQ(brave_sync_prefs()->GetThisDeviceIdV2(), "");

  RecordsList records;
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "", "0", "",
                         "device0"));
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "", "1", "",
                         "device1"));
  sync_service()->OnResolvedPreferences(records);
  brave_sync_prefs()->SetLastFetchTime(base::Time::Now());

  auto devices = brave_sync_prefs()->GetSyncDevices();

  EXPECT_TRUE(DevicesContains(devices.get(), "0", "", "device0"));
  EXPECT_TRUE(DevicesContains(devices.get(), "1", "", "device1"));

  const std::string device_id_v2 = "beef00";
  // sync library will call SAVE_INIT_DATA to propagate device id v2
  sync_service()->OnSaveInitData(binary_seed, {0}, device_id_v2);
  EXPECT_EQ(brave_sync_prefs()->GetThisDeviceIdV2(), device_id_v2);

  EXPECT_CALL(*sync_client(),
              SendSyncRecords("PREFERENCES",
                              ContainsDeviceRecord(SyncRecord::Action::A_DELETE,
                                                   "device0", device_id_v2)))
  .Times(1);
  EXPECT_CALL(*sync_client(),
              SendSyncRecords("PREFERENCES",
                              ContainsDeviceRecord(SyncRecord::Action::A_CREATE,
                                                   "device0", device_id_v2)))
  .Times(1);

  base::WaitableEvent we;
  brave_sync::GetRecordsCallback on_get_records =
      base::BindOnce(&OnGetRecordsStub);
  sync_service()->OnPollSyncCycle(std::move(on_get_records), &we);
}

TEST_F(BraveSyncServiceTest, DeviceIdV2MigrationDupDeviceId) {
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged);
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service()))
      .Times(AtLeast(2));
  sync_service()->OnSetupSyncHaveCode(kTestWords1, "device1");
  EXPECT_TRUE(
      profile()->GetPrefs()->GetBoolean(brave_sync::prefs::kSyncEnabled));

  // Service gets seed from client via BraveSyncServiceImpl::OnSaveInitData
  const auto binary_seed = brave_sync::Uint8Array(16, 77);

  // emulate device with dup device id and without device id v2
  sync_service()->OnSaveInitData(binary_seed, {1}, "");
  std::string expected_seed = brave_sync::StrFromUint8Array(binary_seed);
  EXPECT_EQ(sync_service()->GetSeed(), expected_seed);
  EXPECT_EQ(brave_sync_prefs()->GetThisDeviceIdV2(), "");

  RecordsList records;
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "", "0", "",
                         "device0"));
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "", "1", "",
                         "device1"));
  records.push_back(
      SimpleDeviceRecord(SyncRecord::Action::A_CREATE, "", "1", "",
                         "device2"));
  sync_service()->OnResolvedPreferences(records);
  brave_sync_prefs()->SetLastFetchTime(base::Time::Now());

  auto devices = brave_sync_prefs()->GetSyncDevices();

  EXPECT_TRUE(DevicesContains(devices.get(), "0", "", "device0"));
  EXPECT_TRUE(DevicesContains(devices.get(), "1", "", "device1"));
  EXPECT_TRUE(DevicesContains(devices.get(), "1", "", "device2"));

  const std::string device_id_v2 = "beef01";
  // sync library will call SAVE_INIT_DATA to propagate device id v2
  sync_service()->OnSaveInitData(binary_seed, {1}, device_id_v2);
  EXPECT_EQ(brave_sync_prefs()->GetThisDeviceIdV2(), device_id_v2);

  // It will send DELETE record twice because two device records has same device
  // id
  EXPECT_CALL(*sync_client(),
              SendSyncRecords("PREFERENCES",
                              ContainsDeviceRecord(SyncRecord::Action::A_DELETE,
                                                   "device1", device_id_v2)))
  .Times(2);
  EXPECT_CALL(*sync_client(),
              SendSyncRecords("PREFERENCES",
                              ContainsDeviceRecord(SyncRecord::Action::A_CREATE,
                                                   "device1", device_id_v2)))
  .Times(1);

  base::WaitableEvent we;
  brave_sync::GetRecordsCallback on_get_records =
      base::BindOnce(&OnGetRecordsStub);
  sync_service()->OnPollSyncCycle(std::move(on_get_records), &we);
}

TEST_F(BraveSyncServiceTest, IsOtherBookmarksFolder) {
  AsMutable(model()->other_node())
    ->SetMetaInfo("object_id", GenerateObjectIdForOtherNode(std::string()));
  auto record_matches_id = SimpleFolderSyncRecord(
      SyncRecord::Action::A_CREATE, GenerateObjectIdForOtherNode(std::string()),
      "", "", "", "", false, "");
  EXPECT_TRUE(sync_service()->IsOtherBookmarksFolder(record_matches_id.get()));

  auto record_matches_traits = SimpleFolderSyncRecord(
      SyncRecord::Action::A_CREATE, GenerateObjectIdForOtherNode("123"),
      kOtherNodeName, kOtherNodeOrder, "", "", false, kOtherNodeName);
  EXPECT_TRUE(sync_service()
                ->IsOtherBookmarksFolder(record_matches_traits.get()));

  auto record_not_match1 = SimpleFolderSyncRecord(
      SyncRecord::Action::A_CREATE, GenerateObjectIdForOtherNode("123"),
      kOtherNodeName, "", "", "", false, kOtherNodeName);
  EXPECT_FALSE(sync_service()
                ->IsOtherBookmarksFolder(record_not_match1.get()));

  auto record_not_match2 = SimpleFolderSyncRecord(
      SyncRecord::Action::A_CREATE, GenerateObjectIdForOtherNode("123"),
      "What Bookmarks", kOtherNodeOrder, "", "", false, kOtherNodeName);
  EXPECT_FALSE(sync_service()
                ->IsOtherBookmarksFolder(record_not_match2.get()));

  auto record_not_match3 = SimpleFolderSyncRecord(
      SyncRecord::Action::A_CREATE, GenerateObjectIdForOtherNode("123"),
      kOtherNodeName, kOtherNodeOrder, "", "", false, "No Bookmarks");
  EXPECT_FALSE(sync_service()
                ->IsOtherBookmarksFolder(record_not_match3.get()));

  auto record_not_folder = SimpleBookmarkSyncRecord(
      SyncRecord::Action::A_CREATE, GenerateObjectIdForOtherNode(std::string()),
      "", "", "", "", "", false);
  EXPECT_FALSE(sync_service()
                ->IsOtherBookmarksFolder(record_not_folder.get()));
}

TEST_F(BraveSyncServiceTest, ProcessOtherBookmarksFolder) {
  const std::string object_id_iter1 =
    GenerateObjectIdForOtherNode(std::string());
  auto record1 = SimpleFolderSyncRecord(
      SyncRecord::Action::A_CREATE, object_id_iter1,
      kOtherNodeName, kOtherNodeOrder, "", "", false, kOtherNodeName);
  EXPECT_TRUE(sync_service()->IsOtherBookmarksFolder(record1.get()));

  bool pass_to_syncer = false;
  sync_service()->ProcessOtherBookmarksFolder(record1.get(), &pass_to_syncer);
  EXPECT_FALSE(pass_to_syncer);
  std::string other_node_object_id;
  model()->other_node()->GetMetaInfo("object_id", &other_node_object_id);
  EXPECT_EQ(other_node_object_id, object_id_iter1);

  const std::string object_id_iter2 =
    GenerateObjectIdForOtherNode(object_id_iter1);
  // Now we emulate our object id is out-dated, need to catch up with current
  // one
  auto record2 = SimpleFolderSyncRecord(
      SyncRecord::Action::A_CREATE, object_id_iter2,
      kOtherNodeName, kOtherNodeOrder, "", "", false, kOtherNodeName);
  EXPECT_TRUE(sync_service()->IsOtherBookmarksFolder(record2.get()));

  pass_to_syncer = false;
  sync_service()->ProcessOtherBookmarksFolder(record2.get(), &pass_to_syncer);
  EXPECT_FALSE(pass_to_syncer);
  other_node_object_id = "";
  model()->other_node()->GetMetaInfo("object_id", &other_node_object_id);
  EXPECT_EQ(other_node_object_id, object_id_iter2);

  // Prepare children of other_node()
  const bookmarks::BookmarkNode* folder_a =
    model()->AddFolder(model()->other_node(), 0, base::ASCIIToUTF16("A"));
  const bookmarks::BookmarkNode* bookmark_a1 =
    model()->AddURL(model()->other_node(), 1, base::ASCIIToUTF16("A1"),
                    GURL("https://a1.com"));
  const std::string folder_a_object_id = GenerateObjectId();
  const std::string bookmark_a1_object_id = GenerateObjectId();
  AsMutable(folder_a)->SetMetaInfo("object_id", folder_a_object_id);
  AsMutable(bookmark_a1)->SetMetaInfo("object_id", bookmark_a1_object_id);
  AsMutable(folder_a)->SetMetaInfo("order",
                                   std::string(kOtherNodeOrder) + ".1");
  AsMutable(bookmark_a1)->SetMetaInfo("order",
                                      std::string(kOtherNodeOrder) + ".2");
  AsMutable(folder_a)
    ->SetMetaInfo("sync_timestamp",
                  std::to_string(base::Time::Now().ToJsTime()));
  AsMutable(bookmark_a1)
    ->SetMetaInfo("sync_timestamp",
                  std::to_string(base::Time::Now().ToJsTime()));

  // Prepare a folder for "Other Bookmarks" to move to
  const bookmarks::BookmarkNode* folder_b =
    model()->AddFolder(model()->bookmark_bar_node(), 0,
                       base::ASCIIToUTF16("B"));
  const std::string folder_b_object_id = GenerateObjectId();
  AsMutable(folder_b)->SetMetaInfo("object_id", folder_b_object_id);
  AsMutable(folder_b)->SetMetaInfo("order", "1.0.1.1");
  AsMutable(folder_b)
    ->SetMetaInfo("sync_timestamp",
                  std::to_string(base::Time::Now().ToJsTime()));
  // ==========================================================================
  // Emulate MOVE
  const std::string object_id_iter3 =
    GenerateObjectIdForOtherNode(object_id_iter2);
  auto record3 = SimpleFolderSyncRecord(
      SyncRecord::Action::A_CREATE, object_id_iter2,
      kOtherNodeName, "1.0.1.1.1", folder_b_object_id, "",
      false, kOtherNodeName);
  EXPECT_TRUE(sync_service()->IsOtherBookmarksFolder(record3.get()));

  auto record_a = SimpleFolderSyncRecord(
      SyncRecord::Action::A_UPDATE, folder_a_object_id,
      "A", "1.0.1.1.1.1", object_id_iter2, "",
      false, "A");

  auto record_a1 = SimpleBookmarkSyncRecord(
      SyncRecord::Action::A_UPDATE, bookmark_a1_object_id, "https://a1.com/",
      "A1", "1.0.1.1.1.2", object_id_iter2, "", false);

  // Expect sending updates
  auto record_a_to_send = SyncRecord::Clone(*record_a);
  auto record_a1_to_send = SyncRecord::Clone(*record_a1);
  RecordsListPtr records_to_send = std::make_unique<RecordsList>();
  records_to_send->push_back(std::move(record_a_to_send));
  records_to_send->push_back(std::move(record_a1_to_send));
  EXPECT_CALL(*sync_client(),
              SendSyncRecords(kBookmarks,
                              MatchBookmarksRecords(
                                std::move(records_to_send.get()))))
    .Times(1);

  pass_to_syncer = false;
  sync_service()->ProcessOtherBookmarksFolder(record3.get(), &pass_to_syncer);
  EXPECT_TRUE(pass_to_syncer);
  other_node_object_id = "";
  model()->other_node()->GetMetaInfo("object_id", &other_node_object_id);
  EXPECT_EQ(other_node_object_id, object_id_iter3);

  // Move folder A to "Other Bookmark" folder under folder B
  EXPECT_EQ(sync_service()->pending_received_records_->size(), 2u);
  EXPECT_TRUE(sync_service()->pending_received_records_->at(0)
            ->GetBookmark().Matches(record_a->GetBookmark()));
  EXPECT_TRUE(sync_service()->pending_received_records_->at(1)
            ->GetBookmark().Matches(record_a1->GetBookmark()));
  // ==========================================================================
  // Emulate RENAME
  sync_service()->pending_received_records_->clear();
  const std::string object_id_iter4 =
    GenerateObjectIdForOtherNode(object_id_iter3);
  const std::string new_other_node_name = "Mother Bookmarks";
  auto record4 = SimpleFolderSyncRecord(
      SyncRecord::Action::A_CREATE, object_id_iter3,
      new_other_node_name, kOtherNodeOrder, "", "",
      false, new_other_node_name);
  EXPECT_TRUE(sync_service()->IsOtherBookmarksFolder(record4.get()));

  record_a = SimpleFolderSyncRecord(
      SyncRecord::Action::A_UPDATE, folder_a_object_id,
      "A", std::string(kOtherNodeOrder) + ".1", object_id_iter3, "",
      false, "A");
  record_a1 = SimpleBookmarkSyncRecord(
      SyncRecord::Action::A_UPDATE, bookmark_a1_object_id, "https://a1.com/",
      "A1", std::string(kOtherNodeOrder) + ".2", object_id_iter3, "", false);

  // Expect sending updates
  record_a_to_send = SyncRecord::Clone(*record_a);
  record_a1_to_send = SyncRecord::Clone(*record_a1);
  records_to_send = std::make_unique<RecordsList>();
  records_to_send->push_back(std::move(record_a_to_send));
  records_to_send->push_back(std::move(record_a1_to_send));
  EXPECT_CALL(*sync_client(),
              SendSyncRecords(kBookmarks,
                              MatchBookmarksRecords(
                                std::move(records_to_send.get()))))
    .Times(1);

  pass_to_syncer = false;
  sync_service()->ProcessOtherBookmarksFolder(record4.get(), &pass_to_syncer);
  EXPECT_TRUE(pass_to_syncer);
  other_node_object_id = "";
  model()->other_node()->GetMetaInfo("object_id", &other_node_object_id);
  EXPECT_EQ(other_node_object_id, object_id_iter4);

  // Move folder A to "Mother Bookmark" folder
  EXPECT_EQ(sync_service()->pending_received_records_->size(), 2u);
  EXPECT_TRUE(sync_service()->pending_received_records_->at(0)
            ->GetBookmark().Matches(record_a->GetBookmark()));
  EXPECT_TRUE(sync_service()->pending_received_records_->at(1)
            ->GetBookmark().Matches(record_a1->GetBookmark()));
  // ==========================================================================
  // Emulate REORDER (which will be ignored)
  sync_service()->pending_received_records_->clear();
  auto record5 = SimpleFolderSyncRecord(
      SyncRecord::Action::A_CREATE, object_id_iter4,
      kOtherNodeName, "1.0.1.2", "", "",
      false, kOtherNodeName);
  EXPECT_TRUE(sync_service()->IsOtherBookmarksFolder(record5.get()));

  EXPECT_CALL(*sync_client(), SendSyncRecords(kBookmarks, _)).Times(0);

  pass_to_syncer = false;
  sync_service()->ProcessOtherBookmarksFolder(record5.get(), &pass_to_syncer);
  EXPECT_FALSE(pass_to_syncer);
  other_node_object_id = "";
  model()->other_node()->GetMetaInfo("object_id", &other_node_object_id);
  EXPECT_EQ(other_node_object_id, object_id_iter4);

  EXPECT_EQ(sync_service()->pending_received_records_->size(), 0u);
}

TEST_F(BraveSyncServiceTest, ProcessOtherBookmarksChildren) {
  AsMutable(model()->other_node())
    ->SetMetaInfo("object_id", GenerateObjectIdForOtherNode(std::string()));
  auto record_a = SimpleFolderSyncRecord(
      SyncRecord::Action::A_CREATE, "",
      "A", std::string(kOtherNodeOrder) + ".1",
      GenerateObjectIdForOtherNode(std::string()), "",
      false, "A");
  EXPECT_FALSE(record_a->GetBookmark().hideInToolbar);
  sync_service()->ProcessOtherBookmarksChildren(record_a.get());
  EXPECT_TRUE(record_a->GetBookmark().hideInToolbar);

  auto record_a1 = SimpleBookmarkSyncRecord(
      SyncRecord::Action::A_CREATE, "", "https://a1.com/",
      "A1", "1.1.1.1", "", "", false);
  EXPECT_FALSE(record_a1->GetBookmark().hideInToolbar);
  sync_service()->ProcessOtherBookmarksChildren(record_a1.get());
  EXPECT_FALSE(record_a1->GetBookmark().hideInToolbar);
}

TEST_F(BraveSyncServiceTest, CheckOtherBookmarkRecord) {
  const std::string object_id_iter1 =
    GenerateObjectIdForOtherNode(std::string());
  const std::string object_id_iter2 =
    GenerateObjectIdForOtherNode(object_id_iter1);
  // No object id for other_node yet
  auto record = SimpleFolderSyncRecord(
      SyncRecord::Action::A_CREATE, "",
      kOtherNodeName, kOtherNodeOrder, "", "",
      false, kOtherNodeName);
  std::string other_node_object_id;
  EXPECT_FALSE(model()->other_node()->GetMetaInfo("object_id",
                                                  &other_node_object_id));
  sync_service()->CheckOtherBookmarkRecord(record.get());
  EXPECT_TRUE(model()->other_node()->GetMetaInfo("object_id",
                                                  &other_node_object_id));
  EXPECT_EQ(other_node_object_id, object_id_iter1);
  EXPECT_EQ(record->objectId, other_node_object_id);

  // Object id is out-dated
  record = SimpleFolderSyncRecord(
      SyncRecord::Action::A_CREATE, object_id_iter1,
      kOtherNodeName, kOtherNodeOrder, "", "",
      false, kOtherNodeName);
  AsMutable(model()->other_node())
    ->SetMetaInfo("object_id", object_id_iter2);
  sync_service()->CheckOtherBookmarkRecord(record.get());
  EXPECT_EQ(record->objectId, object_id_iter2);
}

TEST_F(BraveSyncServiceTest, CheckOtherBookmarkChildRecord) {
  const std::string object_id_iter1 =
    GenerateObjectIdForOtherNode(std::string());
  AsMutable(model()->other_node())
    ->SetMetaInfo("object_id", object_id_iter1);
  auto record_a1 = SimpleBookmarkSyncRecord(
      SyncRecord::Action::A_CREATE, "", "https://a1.com/",
      "A1", std::string(kOtherNodeOrder) + ".1" , "", "", true);
  EXPECT_TRUE(record_a1->GetBookmark().parentFolderObjectId.empty());
  sync_service()->CheckOtherBookmarkChildRecord(record_a1.get());
  EXPECT_EQ(record_a1->GetBookmark().parentFolderObjectId, object_id_iter1);
}

TEST_F(BraveSyncServiceTest, AddNonClonedBookmarkKeys) {
  sync_service()->AddNonClonedBookmarkKeys(model());
  const bookmarks::BookmarkNode* bookmark_a1 =
      model()->AddURL(model()->other_node(), 0, base::ASCIIToUTF16("A1"),
                      GURL("https://a1.com"));

  AsMutable(bookmark_a1)->SetMetaInfo("object_id", "object_id_value");
  AsMutable(bookmark_a1)->SetMetaInfo("order", "order_value");
  AsMutable(bookmark_a1)->SetMetaInfo("sync_timestamp", "sync_timestamp_value");
  AsMutable(bookmark_a1)->SetMetaInfo("version", "version_value");

  model()->Copy(bookmark_a1, model()->other_node(), 1);

  const bookmarks::BookmarkNode* bookmark_copy =
      model()->other_node()->children().at(1).get();

  std::string meta_object_id;
  EXPECT_FALSE(bookmark_copy->GetMetaInfo("object_id", &meta_object_id));
  EXPECT_TRUE(meta_object_id.empty());
  std::string meta_order;
  EXPECT_FALSE(bookmark_copy->GetMetaInfo("order", &meta_order));
  EXPECT_TRUE(meta_order.empty());
  std::string meta_sync_timestamp;
  EXPECT_FALSE(
      bookmark_copy->GetMetaInfo("sync_timestamp", &meta_sync_timestamp));
  EXPECT_TRUE(meta_sync_timestamp.empty());
  std::string meta_version;
  EXPECT_FALSE(bookmark_copy->GetMetaInfo("version", &meta_version));
  EXPECT_TRUE(meta_version.empty());
}

namespace {

void SetBraveMeta(const bookmarks::BookmarkNode* node,
                  const std::string& object_id,
                  const std::string& order,
                  const std::string& sync_timestamp,
                  const std::string& version) {
  bookmarks::BookmarkNode* mutable_node = AsMutable(node);
  mutable_node->SetMetaInfo("object_id", object_id);
  mutable_node->SetMetaInfo("order", order);
  mutable_node->SetMetaInfo("sync_timestamp", sync_timestamp);
  mutable_node->SetMetaInfo("version", version);
}

void GetAllNodes(const bookmarks::BookmarkNode* parent,
                 std::set<const bookmarks::BookmarkNode*>* all_nodes) {
  for (size_t i = 0; i < parent->children().size(); ++i) {
    const bookmarks::BookmarkNode* current_child = parent->children()[i].get();
    all_nodes->insert(current_child);
    if (current_child->is_folder()) {
      GetAllNodes(current_child, all_nodes);
    }
  }
}

}  //  namespace

TEST_F(BraveSyncServiceTest, MigrateDuplicatedBookmarksObjectIds) {
  AsMutable(model()->other_node())->SetMetaInfo("order", kOtherNodeOrder);

  const bookmarks::BookmarkNode* bookmark_a1 =
      model()->AddURL(model()->other_node(), 0, base::ASCIIToUTF16("A1"),
                      GURL("https://a1.com"));

  SetBraveMeta(bookmark_a1, "object_id_value", "255.255.255.3",
               "sync_timestamp_value", "version_value");

  model()->Copy(bookmark_a1, model()->other_node(), 1);

  model()->Copy(bookmark_a1, model()->other_node(), 2);

  const bookmarks::BookmarkNode* folder_f1 =
      model()->AddFolder(model()->other_node(), 3, base::ASCIIToUTF16("F1"));
  SetBraveMeta(folder_f1, "object_id_value", "255.255.255.5",
               "sync_timestamp_value", "version_value");

  const bookmarks::BookmarkNode* bookmark_b1 = model()->AddURL(
      folder_f1, 0, base::ASCIIToUTF16("B1"), GURL("https://b1.com"));
  SetBraveMeta(bookmark_b1, "object_id_value", "255.255.255.5.1",
               "sync_timestamp_value", "version_value");

  model()->Copy(folder_f1, model()->other_node(), 4);
  model()->Copy(folder_f1, model()->other_node(), 5);
  model()->Move(model()->other_node()->children()[5].get(), folder_f1, 0);

  std::set<const bookmarks::BookmarkNode*> all_nodes;
  GetAllNodes(model()->other_node(), &all_nodes);
  for (const bookmarks::BookmarkNode* node : all_nodes) {
    // Verify fields after copying
    std::string meta_object_id;
    EXPECT_TRUE(node->GetMetaInfo("object_id", &meta_object_id));
    EXPECT_EQ(meta_object_id, "object_id_value");
    std::string meta_sync_timestamp;
    EXPECT_TRUE(node->GetMetaInfo("sync_timestamp", &meta_sync_timestamp));
    EXPECT_EQ(meta_sync_timestamp, "sync_timestamp_value");
    std::string meta_version;
    EXPECT_TRUE(node->GetMetaInfo("version", &meta_version));
    EXPECT_EQ(meta_version, "version_value");

    // Simulate all bookmarks don`t have added time, as a worse case,
    // but happened on live profile
    AsMutable(node)->set_date_added(base::Time());
  }

  sync_service()->AddNonClonedBookmarkKeys(model());

  // Do the migration
  BraveProfileSyncServiceImpl::MigrateDuplicatedBookmarksObjectIds(profile(),
                                                                   model());

  // All the bookmarks after migration must not have sync meta info
  all_nodes.clear();
  GetAllNodes(model()->other_node(), &all_nodes);
  for (const bookmarks::BookmarkNode* bookmark : all_nodes) {
    std::string migrated_object_id;
    std::string migrated_order;
    std::string migrated_sync_timestamp;
    std::string migrated_version;
    EXPECT_FALSE(bookmark->GetMetaInfo("object_id", &migrated_object_id));
    EXPECT_FALSE(bookmark->GetMetaInfo("order", &migrated_order));
    EXPECT_FALSE(
        bookmark->GetMetaInfo("sync_timestamp", &migrated_sync_timestamp));
    EXPECT_FALSE(bookmark->GetMetaInfo("version", &migrated_version));
  }
}
