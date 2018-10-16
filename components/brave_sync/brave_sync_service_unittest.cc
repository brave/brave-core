/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/files/file_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"

#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/components/brave_sync/client/brave_sync_client.h"
//#include "brave/components/brave_sync/client/brave_sync_client_factory.h"
#include "brave/components/brave_sync/client/client_ext_impl_data.h"
#include "brave/components/brave_sync/brave_sync_service.h"
#include "brave/components/brave_sync/brave_sync_service_factory.h"
#include "brave/components/brave_sync/brave_sync_service_impl.h"
//#include "brave/components/brave_sync/debug.h"
#include "brave/components/brave_sync/jslib_messages.h"

#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile.h"
//#include "chrome/test/base/browser_with_test_window_test.h" // error: undefined symbol: BrowserWithTestWindowTest::SetUp()

#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"

#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/test/test_bookmark_client.h"
#include "components/sync_preferences/pref_service_mock_factory.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/extension_system.h"

#include "gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveSyncServiceTest.*

// BraveSyncClient::methods
// Name                     | Covered
//------------------------------------
// SetSyncToBrowserHandler  |
// GetSyncToBrowserHandler  |
// SendGotInitData          |
// SendFetchSyncRecords     |
// SendFetchSyncDevices     |
// SendResolveSyncRecords   |
// SendSyncRecords          |
// SendDeleteSyncUser       | ?
// SendDeleteSyncCategory   | ?
// SendGetBookmarksBaseOrder|
// SendGetBookmarkOrder     |
// NeedSyncWords            | ?
// NeedBytesFromSyncWords   | ?
// OnExtensionInitialized   |

using ::testing::_;
using ::testing::AtLeast;
using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;
using bookmarks::TestBookmarkClient;
using brave_sync::BraveSyncService;
using brave_sync::BraveSyncServiceImpl;
using brave_sync::BraveSyncServiceFactory;
using brave_sync::SyncRecordAndExistingList;
using brave_sync::SyncMessageHandler;
using brave_sync::Uint8Array;
using brave_sync::RecordsList;

class MockBraveSyncClient : public brave_sync::BraveSyncClient {
 public:
  MockBraveSyncClient() {
    DLOG(INFO) << "[Brave Sync Test] MockBraveSyncClient ctor this="<<this;
  }

  MOCK_METHOD0(sync_message_handler, SyncMessageHandler*());
  MOCK_METHOD4(SendGotInitData, void(const Uint8Array& seed,
    const Uint8Array& device_id, const brave_sync::client_data::Config& config,
    const std::string& sync_words));
  MOCK_METHOD3(SendFetchSyncRecords, void(
    const std::vector<std::string>& category_names, const base::Time& startAt,
    const int max_records));
  MOCK_METHOD0(SendFetchSyncDevices, void());
  MOCK_METHOD2(SendResolveSyncRecords, void(const std::string& category_name,
    std::unique_ptr<SyncRecordAndExistingList> list));
  MOCK_METHOD2(SendSyncRecords, void (const std::string& category_name,
    const RecordsList& records));
  MOCK_METHOD0(SendDeleteSyncUser, void());
  MOCK_METHOD1(SendDeleteSyncCategory, void(const std::string& category_name));
  MOCK_METHOD2(SendGetBookmarksBaseOrder, void(const std::string& device_id,
    const std::string& platform));
  MOCK_METHOD3(SendGetBookmarkOrder, void(const std::string& prevOrder,
    const std::string& nextOrder, const std::string& parent_order));
  MOCK_METHOD1(NeedSyncWords, void(const std::string& seed));
  MOCK_METHOD1(NeedBytesFromSyncWords, void(const std::string& words));
  MOCK_METHOD0(OnExtensionInitialized, void());
  MOCK_METHOD0(OnSyncEnabledChanged, void());
};

Profile* CreateBraveSyncProfile(
    const base::FilePath& path, Profile::Delegate* delegate) {
  TestingProfile::Builder profile_builder;
  sync_preferences::PrefServiceMockFactory factory;
  auto registry = base::MakeRefCounted<user_prefs::PrefRegistrySyncable>();
  std::unique_ptr<sync_preferences::PrefServiceSyncable> prefs(
      factory.CreateSyncable(registry.get()));
  RegisterUserProfilePrefs(registry.get());

  // ^ for the first test this is is required, for the 2nd test already done in (below), so it DCHECKs
  // (^) if I enabled BraveSyncServiceFactory::RegisterProfilePrefsStatic for the second run
  // #5 0x000003537941 brave_sync::BraveSyncServiceFactory::RegisterProfilePrefsStatic()
  // #6 0x000003538409 brave_sync::BraveSyncServiceFactory::RegisterProfilePrefs()
  // #7 0x7f5d225a4b6c BrowserContextKeyedServiceFactory::RegisterPrefs()
  // #8 0x7f5d23af30c1 KeyedServiceBaseFactory::RegisterPrefsIfNecessaryForContext()
  // #9 0x7fd1d62f37a7 BrowserContextKeyedServiceFactory::GetServiceForBrowserContext()*****
  // #10 0x000003537487 brave_sync::BraveSyncServiceFactory::GetForProfile()
  // #11 0x000001dc13d2 BraveSyncServiceTest::SetUp()

  // ^ if BraveSyncServiceFactory::RegisterProfilePrefsStatic is  disabled for the first run
  // #4 0x000003589ad1 brave_sync::prefs::Prefs::GetSeed()
  // #5 0x00000353983b brave_sync::BraveSyncServiceImpl::BraveSyncServiceImpl()
  // #6 0x0000035375ca brave_sync::BraveSyncServiceFactory::BuildServiceInstanceFor()
  // #7 0x7fa82e74f9ec BrowserContextKeyedServiceFactory::BuildServiceInstanceFor()
  // #8 0x7fa82fc9fa98 KeyedServiceFactory::GetServiceForContext()
  // #9 0x7fa82e74f7a7 BrowserContextKeyedServiceFactory::GetServiceForBrowserContext()*****
  // #10 0x000003537487 brave_sync::BraveSyncServiceFactory::GetForProfile()
  // #11 0x000001dc13d2 BraveSyncServiceTest::SetUp()

  profile_builder.SetPrefService(std::move(prefs));
  profile_builder.SetPath(path);
  if (delegate)  { profile_builder.SetDelegate(delegate); }
  std::unique_ptr<TestingProfile> profile = profile_builder.Build();
  return profile.release();
}

std::unique_ptr<KeyedService> BuildFakeBookmarkModelForTests(
    content::BrowserContext* context) {
  // Don't need context, unless we have more than one profile
  return TestBookmarkClient::CreateModel();
}

MockBraveSyncClient* g_mock_brave_sync_client = nullptr;
std::unique_ptr<KeyedService> BuildFakeBraveSyncServiceForTests(
    content::BrowserContext* context) {
  DCHECK_NE(g_mock_brave_sync_client, nullptr);
  return std::unique_ptr<KeyedService>(new BraveSyncServiceImpl(static_cast<Profile*>(context), g_mock_brave_sync_client));
}

namespace brave_sync {
class BraveSyncServiceImplTestAccess {
public:
  static void PretendBackgroundSyncStarted(BraveSyncService* service) {
    BraveSyncServiceImplTestAccess::GetImpl(service)->BackgroundSyncStarted();
  }
  static void PretendBackgroundSyncStopped(BraveSyncService* service) {
    BraveSyncServiceImplTestAccess::GetImpl(service)->BackgroundSyncStopped();
  }
private:
  static BraveSyncServiceImpl* GetImpl(BraveSyncService* service) {
    return static_cast<BraveSyncServiceImpl*>(service);
  }
};
} // namespace brave_sync

class BraveSyncServiceTest : public ::testing::Test /*public BrowserWithTestWindowTest*/ {
 public:
  BraveSyncServiceTest() {
    DLOG(INFO) << "[Brave Sync Test] CTOR";
  }
  ~BraveSyncServiceTest() override {
    DLOG(INFO) << "[Brave Sync Test] DTOR";
  }
 protected:
  void SetUp() override {
    EXPECT_TRUE(temp_dir_.CreateUniqueTempDir());
    profile_.reset(CreateBraveSyncProfile(temp_dir_.GetPath(), nullptr));
    DLOG(INFO) << "[Brave Sync Test] profile=" << profile_.get();
    DLOG(INFO) << "[Brave Sync Test] profile->GetPath()=" << profile_->GetPath();
    DLOG(INFO) << "[Brave Sync Test] profile->GetDebugName()=" <<
                                                       profile_->GetDebugName();
    DLOG(INFO) << "[Brave Sync Test] profile->IsOffTheRecord()=" <<
                                                     profile_->IsOffTheRecord();

    if (!g_mock_brave_sync_client) {
      g_mock_brave_sync_client = new MockBraveSyncClient();
    }

    BookmarkModelFactory::GetInstance()->SetTestingFactory(
       profile_.get(), &BuildFakeBookmarkModelForTests);

    bookmark_model_ = BookmarkModelFactory::GetForBrowserContext(profile_.get());
    DLOG(INFO) << "[Brave Sync Test] bookmark_model=" << bookmark_model_;
    DCHECK(bookmark_model_);

    BraveSyncServiceFactory::GetInstance()->SetTestingFactory(
       profile_.get(), &BuildFakeBraveSyncServiceForTests);

    service_ = BraveSyncServiceFactory::GetInstance()->GetForProfile(profile_.get());
    // ^here DCHECK already configured on the second time; and here the DCHECK if prefs are configured on the first time
    DLOG(INFO) << "[Brave Sync Test] service_=" << service_;
    DCHECK_NE(service_, nullptr);
  }
  void TearDown() override {
    DLOG(INFO) << "[Brave Sync Test] TearDown()";
    DLOG(INFO) << "[Brave Sync Test] firing stop loop";
    brave_sync::BraveSyncServiceImplTestAccess::PretendBackgroundSyncStopped(service_);
    DLOG(INFO) << "[Brave Sync Test] fired stop loop";
  }

  // Need this as a very first member to run tests in UI thread
  // When this is set, class should not install any other MessageLoops, like
  // base::test::ScopedTaskEnvironment
  content::TestBrowserThreadBundle thread_bundle_;

  std::unique_ptr<Profile> profile_;
  BraveSyncService *service_;
  bookmarks::BookmarkModel* bookmark_model_ = nullptr;
  base::ScopedTempDir temp_dir_;
};

TEST_F(BraveSyncServiceTest, BookmarkAdded) {
  DLOG(INFO) << "[Brave Sync Test] TEST_F BookmarkAdded 000";

  // BraveSyncService: real
  // BraveSyncClient: mock

  // Invoke BraveSyncService::BookmarkAdded
  // Expect BraveSyncClient::SendGetBookmarkOrder invoked

  EXPECT_CALL(*g_mock_brave_sync_client, OnSyncEnabledChanged())
      .Times(1);

  EXPECT_CALL(*g_mock_brave_sync_client, SendGetBookmarkOrder(_,_,_))
      .Times(AtLeast(1));

  service_->OnSetupSyncNewToSync("UnitTestBookmarkAdded");

  DLOG(INFO) << "[Brave Sync Test] firing start loop";
  brave_sync::BraveSyncServiceImplTestAccess::PretendBackgroundSyncStarted(
                                                                      service_);
  DLOG(INFO) << "[Brave Sync Test] fired start loop";

  // DLOG(INFO) << "[Brave Sync Test] IsSyncConfigured()=" <<
  //                                              service_impl->IsSyncConfigured();
  // DLOG(INFO) << "[Brave Sync Test] IsSyncInitialized()=" <<
  //                                             service_impl->IsSyncInitialized();
  // DCHECK(service_impl->IsSyncConfigured());
  // DCHECK(service_impl->IsSyncInitialized());
  // ^- expected fails, because mock client does not do SYNC_SETUP_ERROR /
  //  GET_INIT_DATA / SAVE_INIT_DATA / SYNC_READY

  bookmarks::AddIfNotBookmarked(bookmark_model_,
                                 GURL("https://a.com"),
                                 base::ASCIIToUTF16("A.com - title"));
  DLOG(INFO) << "[Brave Sync Test] TEST_F 001";
}
