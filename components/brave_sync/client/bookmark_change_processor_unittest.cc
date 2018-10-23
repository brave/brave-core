/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/files/scoped_temp_dir.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_sync/client/bookmark_change_processor.h"
#include "brave/components/brave_sync/client/brave_sync_client_impl.h"
#include "brave/components/brave_sync/client/client_ext_impl_data.h"
#include "brave/components/brave_sync/brave_sync_service_impl.h"
#include "brave/components/brave_sync/brave_sync_service_factory.h"
#include "brave/components/brave_sync/brave_sync_service_observer.h"
#include "brave/components/brave_sync/jslib_const.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/test_util.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/test/test_bookmark_client.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveBookmarkChangeProcessorTest.*

using testing::_;
using testing::AtLeast;
using namespace brave_sync;
using namespace bookmarks;

MATCHER_P2(ContainsRecord, action, location,
    "contains sync record with params") {
  for (const auto& record : arg) {
    if (record->has_bookmark()) {
      const auto& bookmark = record->GetBookmark();
      if (record->action == action &&
          bookmark.site.location == location) {
        return true;
      }
    }
  }
  return false;
}

class BraveBookmarkChangeProcessorTest : public testing::Test {
 public:
  BraveBookmarkChangeProcessorTest() {}
  ~BraveBookmarkChangeProcessorTest() override {}

 protected:
  void SetUp() override {
    EXPECT_TRUE(temp_dir_.CreateUniqueTempDir());

    profile_ = CreateBraveSyncProfile(temp_dir_.GetPath());
    EXPECT_TRUE(profile_.get() != NULL);

    sync_client_.reset(new MockBraveSyncClient()) ;

    BookmarkModelFactory::GetInstance()->SetTestingFactory(
       profile_.get(), &BuildFakeBookmarkModelForTests);

    model_ = BookmarkModelFactory::GetForBrowserContext(
        Profile::FromBrowserContext(profile_.get()));

    sync_prefs_.reset(new brave_sync::prefs::Prefs(profile_->GetPrefs()));

    change_processor_.reset(BookmarkChangeProcessor::Create(
        profile_.get(),
        sync_client(),
        sync_prefs_.get()
    ));

    EXPECT_NE(sync_client(), nullptr);
    EXPECT_NE(bookmark_client(), nullptr);
    EXPECT_NE(model(), nullptr);
    EXPECT_NE(change_processor(), nullptr);
  }

  void TearDown() override {
    change_processor()->Stop();
    change_processor_.reset();
    InvalidateBookmarkChangeProcessor();
    profile_.reset();
  }

  MockBraveSyncClient* sync_client() { return sync_client_.get(); }
  BookmarkClient* bookmark_client() { return model_->client(); }
  BookmarkModel* model() { return model_; }
  brave_sync::BookmarkChangeProcessor* change_processor() {
    return change_processor_.get();
  }

 private:
  // Need this as a very first member to run tests in UI thread
  // When this is set, class should not install any other MessageLoops, like
  // base::test::ScopedTaskEnvironment
  content::TestBrowserThreadBundle thread_bundle_;

  std::unique_ptr<MockBraveSyncClient> sync_client_;
  BookmarkModel* model_;  // Not owns
  std::unique_ptr<brave_sync::BookmarkChangeProcessor> change_processor_;
  std::unique_ptr<Profile> profile_;
  std::unique_ptr<brave_sync::prefs::Prefs> sync_prefs_;
  base::ScopedTempDir temp_dir_;
};

TEST_F(BraveBookmarkChangeProcessorTest, /*DISABLED_*/BookmarkAdded) {
  change_processor()->Start();

  EXPECT_CALL(*sync_client(), SendGetBookmarkOrder(_,_,_))
      .Times(AtLeast(1));
  bookmarks::AddIfNotBookmarked(model(),
                                 GURL("https://a.com/"),
                                 base::ASCIIToUTF16("A.com - title"));

  change_processor()->OnGetBookmarkOrder("1.0.4", "", "", "0");
  using brave_sync::jslib::SyncRecord;
  EXPECT_CALL(*sync_client(), SendSyncRecords("BOOKMARKS",
      ContainsRecord(SyncRecord::Action::CREATE, "https://a.com/"))).Times(1);
  change_processor()->SendUnsynced(base::TimeDelta::FromMinutes(10));
}

TEST_F(BraveBookmarkChangeProcessorTest, BookmarkDeleted) {
  change_processor()->Start();

  EXPECT_CALL(*sync_client(), SendGetBookmarkOrder(_,_,_))
      .Times(AtLeast(1));
  bookmarks::AddIfNotBookmarked(model(),
                                 GURL("https://a.com/"),
                                 base::ASCIIToUTF16("A.com - title"));

  change_processor()->OnGetBookmarkOrder("1.0.4", "", "", "0");
  using brave_sync::jslib::SyncRecord;
  EXPECT_CALL(*sync_client(), SendSyncRecords("BOOKMARKS",
      ContainsRecord(SyncRecord::Action::CREATE, "https://a.com/"))).Times(1);
  change_processor()->SendUnsynced(base::TimeDelta::FromMinutes(10));

  // And just now can actually test delete
  std::vector<const bookmarks::BookmarkNode*> nodes;
  bookmarks::GetMostRecentlyAddedEntries(model(), 1, &nodes);
  ASSERT_EQ(nodes.size(), 1u);
  ASSERT_NE(nodes.at(0), nullptr);
  EXPECT_CALL(*sync_client(), SendSyncRecords(_,_)).Times(1);
  model()->Remove(nodes.at(0));  // Crashes for now here
  change_processor()->SendUnsynced(base::TimeDelta::FromMinutes(10));
}
