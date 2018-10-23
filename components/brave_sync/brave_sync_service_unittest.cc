/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/files/scoped_temp_dir.h"
#include "brave/components/brave_sync/client/bookmark_change_processor.h"
#include "brave/components/brave_sync/client/brave_sync_client_impl.h"
#include "brave/components/brave_sync/client/client_ext_impl_data.h"
#include "brave/components/brave_sync/brave_sync_service_impl.h"
#include "brave/components/brave_sync/brave_sync_service_factory.h"
#include "brave/components/brave_sync/brave_sync_service_observer.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/test_util.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/test/test_bookmark_client.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

      // Separated to easier move or remove
      #include "base/strings/utf_string_conversions.h"
      #include "brave/components/brave_sync/jslib_const.h"
      #include "components/bookmarks/browser/bookmark_model.h"
      #include "components/bookmarks/browser/bookmark_utils.h"

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

    DLOG(INFO) << "[Brave Sync Test] service_=" << sync_service_;
    EXPECT_TRUE(sync_service_ != NULL);
  }

  void TearDown() override {
    DLOG(INFO) << "[Brave Sync Test] TearDown()";
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
  // Expect BraveSyncClient::SendGetBookmarkOrder invoked
  // Expect BraveSyncClient::SendSyncRecords invoked
  EXPECT_CALL(*sync_client(), OnSyncEnabledChanged).Times(1);
  EXPECT_CALL(*observer(), OnSyncStateChanged(sync_service())).Times(AtLeast(1));
  sync_service()->OnSetupSyncNewToSync("UnitTestBookmarkAdded");
  DLOG(INFO) << "[Brave Sync Test] firing start loop";
  sync_service()->BackgroundSyncStarted(true/*startup*/);

  DLOG(INFO) << "[Brave Sync Test] fired start loop";
  auto* bookmark_model = BookmarkModelFactory::GetForBrowserContext(profile());
  EXPECT_CALL(*sync_client(), SendGetBookmarkOrder(_,_,_))
      .Times(AtLeast(1));
  bookmarks::AddIfNotBookmarked(bookmark_model,
                                 GURL("https://a.com"),
                                 base::ASCIIToUTF16("A.com - title"));
  // Emulate answer from client, OnSaveBookmarkOrder - not sure, should it
  // be here as test.
  // BookmarkChangeProcessor::PopRRContext emulates response from the mock
  // Seems wrong, I looked on `BookmarkChangeProcessor::PushRRContext`
  // to catch values.
  // parent order "0" is not quite expected, but enough to get further
  sync_service()->OnSaveBookmarkOrder("1.0.4", "", "", "0");
  // Force service send bookmarks and fire the mock
  EXPECT_CALL(*sync_client(), SendSyncRecords(_,_)).Times(1);
  sync_service()->OnResolvedSyncRecords(brave_sync::jslib_const::kBookmarks,
    std::make_unique<RecordsList>());
}

TEST_F(BraveSyncServiceTest, BookmarkAdded) {
  DLOG(INFO) << "[Brave Sync Test] TEST_F BookmarkAdded start";
  BookmarkAddedImpl();
  DLOG(INFO) << "[Brave Sync Test] TEST_F BookmarkAdded done";
}

TEST_F(BraveSyncServiceTest, BookmarkDeleted) {
  DLOG(INFO) << "[Brave Sync Test] TEST_F BookmarkDeleted start";
  BookmarkAddedImpl();
  auto* bookmark_model = BookmarkModelFactory::GetForBrowserContext(profile());

  // And just now can actually test delete
  std::vector<const bookmarks::BookmarkNode*> nodes;
  bookmarks::GetMostRecentlyAddedEntries(bookmark_model, 1, &nodes);
  ASSERT_EQ(nodes.size(), 1u);
  ASSERT_NE(nodes.at(0), nullptr);
  EXPECT_CALL(*sync_client(), SendSyncRecords(_,_)).Times(1); // AB: preciece with mock expect filter
  bookmark_model->Remove(nodes.at(0));
  // record->action = jslib::SyncRecord::Action::DELETE;
  // <= BookmarkNodeToSyncBookmark <= BookmarkChangeProcessor::SendUnsynced
  // <= BraveSyncServiceImpl::OnResolvedSyncRecords
  sync_service()->OnResolvedSyncRecords(brave_sync::jslib_const::kBookmarks,
    std::make_unique<RecordsList>());

  DLOG(INFO) << "[Brave Sync Test] TEST_F BookmarkDeleted done";
}
