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
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/bookmarks/test/test_bookmark_client.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveBookmarkChangeProcessorTest.*

// BookmarkChangeProcessor::methods
// Name                        | Covered
//-------------------------------------
// Create                      | + in SetUp
// Start                       | +
// Stop                        | +
// Reset                       | +
// ApplyChangesFromSyncModel   | +
// GetAllSyncData              | +
// SendUnsynced                | +
// InitialSync                 | N/A

// bookmarks::BookmarkModelObserver overrides:
// Name                        | Covered
// BookmarkModelLoaded         | N/A
// BookmarkModelBeingDeleted   | N/A
// BookmarkNodeMoved           | +
// BookmarkNodeAdded           | N/A
// OnWillRemoveBookmarks       | N/A
// BookmarkNodeRemoved         | +
// BookmarkAllUserNodesRemoved | N/A
// BookmarkNodeChanged         | +
// BookmarkMetaInfoChanged     | +
// BookmarkNodeFaviconChanged  | +

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

MATCHER_P(RecordsNumber, expected_number,
    "contains specified sync record number") {
  return static_cast<int>(arg.size()) == static_cast<int>(expected_number);
}

MATCHER_P(AllRecordsHaveAction, expected_action,
    "all records have expected action") {
  if (arg.empty()) {
    return false;
  }

  for (const auto& record : arg) {
    if (record->action != expected_action) {
      return false;
    }
  }
  return true;
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
       profile_.get(), base::BindRepeating(&BuildFakeBookmarkModelForTests));

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
    profile_.reset();
  }

  MockBraveSyncClient* sync_client() { return sync_client_.get(); }
  BookmarkClient* bookmark_client() { return model_->client(); }
  BookmarkModel* model() { return model_; }
  brave_sync::BookmarkChangeProcessor* change_processor() {
    return change_processor_.get();
  }
  const bookmarks::BookmarkPermanentNode* GetDeletedNodeRoot() {
    return static_cast<const bookmarks::BookmarkPermanentNode*>(
              change_processor_->GetDeletedNodeRoot());
  }
  const bookmarks::BookmarkPermanentNode* GetPendingNodeRoot() {
    return static_cast<const bookmarks::BookmarkPermanentNode*>(
              change_processor_->GetPendingNodeRoot());
  }

  void BookmarkAddedImpl();
  void BookmarkCreatedFromSyncImpl();
  bool HasAnySyncMetaInfo(const BookmarkNode* node);
  void AddSimpleHierarchy(
      const BookmarkNode** folder1, const BookmarkNode** node_a,
      const BookmarkNode** node_b, const BookmarkNode** node_c);

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

TEST_F(BraveBookmarkChangeProcessorTest, StartObserver) {
  // The mark of observer processed: metainfo "last_updated_time" is set
  const auto* node_a = model()->AddURL(model()->other_node(), 0,
                                       base::ASCIIToUTF16("A.com - title"),
                                       GURL("https://a.com/"));
  model()->SetTitle(node_a, base::ASCIIToUTF16("A.com - title - upated"));
  std::string last_updated_time_a;
  node_a->GetMetaInfo("last_updated_time", &last_updated_time_a);
  EXPECT_TRUE(last_updated_time_a.empty());

  change_processor()->Start();

  const auto* node_b = model()->AddURL(model()->other_node(), 0,
                                       base::ASCIIToUTF16("B.com - title"),
                                       GURL("https://b.com/"));
  model()->SetTitle(node_b, base::ASCIIToUTF16("B.com - title - upated"));
  std::string last_updated_time_b;
  node_b->GetMetaInfo("last_updated_time", &last_updated_time_b);
  EXPECT_TRUE(!last_updated_time_b.empty());
}

TEST_F(BraveBookmarkChangeProcessorTest, StopObserver) {
  // The mark of observer processed: metainfo "last_updated_time" is set
  change_processor()->Start();
  const auto* node_a = model()->AddURL(model()->other_node(), 0,
                                       base::ASCIIToUTF16("A.com - title"),
                                       GURL("https://a.com/"));
  model()->SetTitle(node_a, base::ASCIIToUTF16("A.com - title - upated"));
  std::string last_updated_time_a;
  node_a->GetMetaInfo("last_updated_time", &last_updated_time_a);
  EXPECT_TRUE(!last_updated_time_a.empty());

  change_processor()->Stop();

  const auto* node_b = model()->AddURL(model()->other_node(), 0,
                                       base::ASCIIToUTF16("B.com - title"),
                                       GURL("https://b.com/"));
  model()->SetTitle(node_b, base::ASCIIToUTF16("B.com - title - upated"));
  std::string last_updated_time_b;
  node_b->GetMetaInfo("last_updated_time", &last_updated_time_b);
  EXPECT_TRUE(last_updated_time_b.empty());
}

bool BraveBookmarkChangeProcessorTest::HasAnySyncMetaInfo(
                                                    const BookmarkNode* node) {
  DCHECK(node);
  const std::vector<std::string> keys = {"object_id", "order", "sync_timestamp",
      "last_send_time", "last_updated_time", "parent_object_id"};
  for (const auto& key : keys ) {
    std::string value;
    if (node->GetMetaInfo(key, &value) && !value.empty()) {
      return true;
    }
  }
  return false;
}

void BraveBookmarkChangeProcessorTest::AddSimpleHierarchy(
    const BookmarkNode** folder1, const BookmarkNode** node_a,
    const BookmarkNode** node_b, const BookmarkNode** node_c) {
  *folder1 = model()->AddFolder(model()->other_node(), 0,
                               base::ASCIIToUTF16("Folder1"));

  *node_a = model()->AddURL(*folder1, 0,
                           base::ASCIIToUTF16("A.com - title"),
                           GURL("https://a.com/"));

  *node_b = model()->AddURL(*folder1, 1,
                           base::ASCIIToUTF16("B.com - title"),
                           GURL("https://b.com/"));

  *node_c = model()->AddURL(*folder1, 2,
                           base::ASCIIToUTF16("C.com - title"),
                           GURL("https://c.com/"));
}

TEST_F(BraveBookmarkChangeProcessorTest, ResetClearMeta) {
  // Reset does clear of the metainfo, but
  // to fillup the metainfo now need to send it to sync
  change_processor()->Start();

  const BookmarkNode* folder1;
  const BookmarkNode* node_a;
  const BookmarkNode* node_b;
  const BookmarkNode* node_c;
  AddSimpleHierarchy(&folder1, &node_a, &node_b, &node_c);

  EXPECT_CALL(*sync_client(), SendSyncRecords("BOOKMARKS", _)).Times(1);
  EXPECT_CALL(*sync_client(), ClearOrderMap()).Times(1);
  change_processor()->SendUnsynced(base::TimeDelta::FromMinutes(10));

  EXPECT_TRUE(HasAnySyncMetaInfo(folder1));
  EXPECT_TRUE(HasAnySyncMetaInfo(node_a));
  EXPECT_TRUE(HasAnySyncMetaInfo(node_b));
  EXPECT_TRUE(HasAnySyncMetaInfo(node_c));

  model()->AddURL(GetDeletedNodeRoot(), 0,
                  base::ASCIIToUTF16("A.com - title"),
                  GURL("https://a.com/"));
  model()->AddURL(GetPendingNodeRoot(), 0,
                  base::ASCIIToUTF16("A.com - title"),
                  GURL("https://a.com/"));
  EXPECT_FALSE(GetDeletedNodeRoot()->empty());
  EXPECT_FALSE(GetPendingNodeRoot()->empty());

  change_processor()->Reset(true);

  EXPECT_FALSE(HasAnySyncMetaInfo(folder1));
  EXPECT_FALSE(HasAnySyncMetaInfo(node_a));
  EXPECT_FALSE(HasAnySyncMetaInfo(node_b));
  EXPECT_FALSE(HasAnySyncMetaInfo(node_c));
  EXPECT_TRUE(GetDeletedNodeRoot()->empty());
  EXPECT_TRUE(GetPendingNodeRoot()->empty());
}

TEST_F(BraveBookmarkChangeProcessorTest, ResetPreserveMeta) {
  // Reset does clear of the metainfo, but
  // to fillup the metainfo now need to send it to sync
  change_processor()->Start();

  const BookmarkNode* folder1;
  const BookmarkNode* node_a;
  const BookmarkNode* node_b;
  const BookmarkNode* node_c;
  AddSimpleHierarchy(&folder1, &node_a, &node_b, &node_c);

  EXPECT_CALL(*sync_client(), SendSyncRecords("BOOKMARKS", _)).Times(1);
  EXPECT_CALL(*sync_client(), ClearOrderMap()).Times(1);
  change_processor()->SendUnsynced(base::TimeDelta::FromMinutes(10));

  EXPECT_TRUE(HasAnySyncMetaInfo(folder1));
  EXPECT_TRUE(HasAnySyncMetaInfo(node_a));
  EXPECT_TRUE(HasAnySyncMetaInfo(node_b));
  EXPECT_TRUE(HasAnySyncMetaInfo(node_c));

  model()->AddURL(GetDeletedNodeRoot(), 0,
                  base::ASCIIToUTF16("A.com - title"),
                  GURL("https://a.com/"));
  model()->AddURL(GetPendingNodeRoot(), 0,
                  base::ASCIIToUTF16("A.com - title"),
                  GURL("https://a.com/"));
  EXPECT_FALSE(GetDeletedNodeRoot()->empty());
  EXPECT_FALSE(GetPendingNodeRoot()->empty());

  change_processor()->Reset(false);

  EXPECT_TRUE(HasAnySyncMetaInfo(folder1));
  EXPECT_TRUE(HasAnySyncMetaInfo(node_a));
  EXPECT_TRUE(HasAnySyncMetaInfo(node_b));
  EXPECT_TRUE(HasAnySyncMetaInfo(node_c));
  EXPECT_TRUE(GetDeletedNodeRoot()->empty());
  EXPECT_TRUE(GetPendingNodeRoot()->empty());
}


TEST_F(BraveBookmarkChangeProcessorTest, DISABLED_InitialSync) {
  // BookmarkChangeProcessor::InitialSync does not do anything now
  // All work for obtaining order is done in background.js
}

void BraveBookmarkChangeProcessorTest::BookmarkAddedImpl() {
  change_processor()->Start();

  bookmarks::AddIfNotBookmarked(model(),
                                 GURL("https://a.com/"),
                                 base::ASCIIToUTF16("A.com - title"));

  using brave_sync::jslib::SyncRecord;
  EXPECT_CALL(*sync_client(), SendSyncRecords("BOOKMARKS",
      ContainsRecord(SyncRecord::Action::A_CREATE, "https://a.com/"))).Times(1);
  EXPECT_CALL(*sync_client(), ClearOrderMap()).Times(1);
  change_processor()->SendUnsynced(base::TimeDelta::FromMinutes(10));
}

TEST_F(BraveBookmarkChangeProcessorTest, BookmarkAdded) {
  BookmarkAddedImpl();
}

TEST_F(BraveBookmarkChangeProcessorTest, BookmarkDeleted) {
  // Add bookmark first
  BookmarkAddedImpl();
  using brave_sync::jslib::SyncRecord;

  // And just now can actually test delete
  std::vector<const bookmarks::BookmarkNode*> nodes;
  bookmarks::GetMostRecentlyAddedEntries(model(), 1, &nodes);
  ASSERT_EQ(nodes.size(), 1u);
  ASSERT_NE(nodes.at(0), nullptr);
  EXPECT_CALL(*sync_client(), SendSyncRecords("BOOKMARKS",
      ContainsRecord(SyncRecord::Action::A_DELETE, "https://a.com/"))).Times(1);
  model()->Remove(nodes.at(0));
  EXPECT_CALL(*sync_client(), ClearOrderMap()).Times(1);
  change_processor()->SendUnsynced(base::TimeDelta::FromMinutes(10));
  EXPECT_FALSE(GetDeletedNodeRoot()->IsVisible());
}

TEST_F(BraveBookmarkChangeProcessorTest, BookmarkModified) {
  // Add bookmark first
  BookmarkAddedImpl();
  using brave_sync::jslib::SyncRecord;

  // And just now can actually test modify
  std::vector<const bookmarks::BookmarkNode*> nodes;
  bookmarks::GetMostRecentlyAddedEntries(model(), 1, &nodes);
  ASSERT_EQ(nodes.size(), 1u);
  ASSERT_NE(nodes.at(0), nullptr);
  EXPECT_CALL(*sync_client(), SendSyncRecords("BOOKMARKS",
      ContainsRecord(SyncRecord::Action::A_UPDATE, "https://a-m.com/"))).Times(1);
  model()->SetURL(nodes.at(0), GURL("https://a-m.com/"));
  EXPECT_CALL(*sync_client(), ClearOrderMap()).Times(1);
  change_processor()->SendUnsynced(base::TimeDelta::FromMinutes(10));
}

TEST_F(BraveBookmarkChangeProcessorTest, BookmarkMovedInFolder) {
  change_processor()->Start();

  const BookmarkNode* folder1;
  const BookmarkNode* node_a;
  const BookmarkNode* node_b;
  const BookmarkNode* node_c;
  AddSimpleHierarchy(&folder1, &node_a, &node_b, &node_c);

  int intex_a = folder1->GetIndexOf(node_a);
  EXPECT_EQ(intex_a, 0);
  int intex_b = folder1->GetIndexOf(node_b);
  EXPECT_EQ(intex_b, 1);
  int intex_c = folder1->GetIndexOf(node_c);
  EXPECT_EQ(intex_c, 2);

  EXPECT_CALL(*sync_client(), SendSyncRecords("BOOKMARKS", _)).Times(1);
  EXPECT_CALL(*sync_client(), ClearOrderMap()).Times(1);
  change_processor()->SendUnsynced(base::TimeDelta::FromMinutes(10));

  EXPECT_TRUE(HasAnySyncMetaInfo(folder1));
  EXPECT_TRUE(HasAnySyncMetaInfo(node_a));
  EXPECT_TRUE(HasAnySyncMetaInfo(node_b));
  EXPECT_TRUE(HasAnySyncMetaInfo(node_c));

  model()->Move(node_a, folder1, 2);

  intex_a = folder1->GetIndexOf(node_a);
  EXPECT_EQ(intex_a, 1);
  intex_b = folder1->GetIndexOf(node_b);
  EXPECT_EQ(intex_b, 0);
  intex_c = folder1->GetIndexOf(node_c);
  EXPECT_EQ(intex_c, 2);


  // Sould see at least one syncRecord Modified
  using brave_sync::jslib::SyncRecord;
  EXPECT_CALL(*sync_client(), SendSyncRecords("BOOKMARKS",
      AllRecordsHaveAction(SyncRecord::Action::A_UPDATE))).Times(1);
  // BookmarkNodeMoved does not reset "last_send_time" so SendUnsynced
  // ignores order change untill unsynced_send_interval passes,
  // so here below unsynced_send_interval is 0
  EXPECT_CALL(*sync_client(), ClearOrderMap()).Times(1);
  change_processor()->SendUnsynced(base::TimeDelta::FromMinutes(0));
}

TEST_F(BraveBookmarkChangeProcessorTest, DISABLED_MoveNodesBetweenDirs) {
  // 1. Create these:
  // Other
  //   Folder1
  //      a.com
  //   Folder2
  //      b.com
  // 2. Move b.com => Folder1

  change_processor()->Start();

  const auto* folder1 = model()->AddFolder(model()->other_node(), 0,
                                           base::ASCIIToUTF16("Folder1"));

  [[maybe_unused]] const auto* node_a = model()->AddURL(folder1, 0,
                                       base::ASCIIToUTF16("A.com - title"),
                                       GURL("https://a.com/"));

  const auto* folder2 = model()->AddFolder(model()->other_node(), 1,
                                           base::ASCIIToUTF16("Folder2"));
  [[maybe_unused]] const auto* node_b = model()->AddURL(folder2, 0,
                                        base::ASCIIToUTF16("B.com - title"),
                                        GURL("https://b.com/"));

  // Send all created objects
  EXPECT_CALL(*sync_client(), SendSyncRecords("BOOKMARKS",
      RecordsNumber(4))).Times(1);
  EXPECT_CALL(*sync_client(), ClearOrderMap()).Times(1);
  change_processor()->SendUnsynced(base::TimeDelta::FromMinutes(10));
}

TEST_F(BraveBookmarkChangeProcessorTest, DeleteFolderWithNodes) {
  // 1. Create these:
  // Other
  //   Folder1
  //      a.com
  //      b.com
  //      c.com
  // 2. Delete Folder1

  change_processor()->Start();

  const BookmarkNode* folder1;
  const BookmarkNode* node_a;
  const BookmarkNode* node_b;
  const BookmarkNode* node_c;
  AddSimpleHierarchy(&folder1, &node_a, &node_b, &node_c);

  EXPECT_CALL(*sync_client(), SendSyncRecords("BOOKMARKS",
      RecordsNumber(4))).Times(1);
  EXPECT_CALL(*sync_client(), ClearOrderMap()).Times(1);
  change_processor()->SendUnsynced(base::TimeDelta::FromMinutes(10));

  model()->Remove(folder1);
  EXPECT_CALL(*sync_client(), SendSyncRecords("BOOKMARKS",_)).Times(1);
  EXPECT_CALL(*sync_client(), ClearOrderMap()).Times(1);
  change_processor()->SendUnsynced(base::TimeDelta::FromMinutes(10));
}

// Another type of tests with `change_processor()->ApplyChangesFromSyncModel`
// Without any mocks
// May ignore order, because it will be moved into background.js

void BraveBookmarkChangeProcessorTest::BookmarkCreatedFromSyncImpl() {
  change_processor()->Start();

  RecordsList records;
  records.push_back(SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "121, 194, 37, 61, 199, 11, 166, 234, 214, 197, 45, 215, 241, 206, 219, 130",
      "https://a.com/",
      "A.com - title",
      "1.1.1.1", ""));
  records.push_back(SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "",
      "https://b.com/",
      "B.com - title",
      "1.1.1.2", ""));

  change_processor()->ApplyChangesFromSyncModel(records);

  // Expecting we can find the bookmarks in a model
  std::vector<const BookmarkNode*> nodes_a;
  model()->GetNodesByURL(GURL("https://a.com/"), &nodes_a);
  ASSERT_EQ(nodes_a.size(), 1u);
  const auto* node_a = nodes_a.at(0);
  EXPECT_EQ(node_a->url().spec(), "https://a.com/");

  std::vector<const BookmarkNode*> nodes_b;
  model()->GetNodesByURL(GURL("https://b.com/"), &nodes_b);
  ASSERT_EQ(nodes_b.size(), 1u);
  const auto* node_b = nodes_b.at(0);
  EXPECT_EQ(node_b->url().spec(), "https://b.com/");

  EXPECT_EQ(node_a->parent(), node_b->parent());

  int index_a = node_a->parent()->GetIndexOf(node_a);
  EXPECT_NE(index_a, -1);

  int index_b = node_b->parent()->GetIndexOf(node_b);
  EXPECT_NE(index_b, -1);

  EXPECT_LT(index_a, index_b);

  EXPECT_TRUE(profile_->GetPrefs()->GetBoolean(
                                      bookmarks::prefs::kShowBookmarkBar));
}

TEST_F(BraveBookmarkChangeProcessorTest, BookmarkCreatedFromSync) {
  BookmarkCreatedFromSyncImpl();
}

TEST_F(BraveBookmarkChangeProcessorTest, BookmarkRemovedFromSync) {
  BookmarkCreatedFromSyncImpl();

  RecordsList records;
  records.push_back(SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_DELETE,
      "121, 194, 37, 61, 199, 11, 166, 234, 214, 197, 45, 215, 241, 206, 219, 130",
      "https://a.com/",
      "A.com - title",
      "1.1.1.1", ""));

  change_processor()->ApplyChangesFromSyncModel(records);
  std::vector<const BookmarkNode*> nodes_a;
  model()->GetNodesByURL(GURL("https://a.com/"), &nodes_a);
  ASSERT_EQ(nodes_a.size(), 0u);

  std::vector<const BookmarkNode*> nodes_b;
  model()->GetNodesByURL(GURL("https://b.com/"), &nodes_b);
  ASSERT_EQ(nodes_b.size(), 1u);
  const auto* node_b = nodes_b.at(0);
  EXPECT_EQ(node_b->url().spec(), "https://b.com/");
}

TEST_F(BraveBookmarkChangeProcessorTest, NestedFoldersCreatedFromSync) {
  // Create these:
  // Other
  //    Folder1
  //       Folder2
  //          Folder3
  //              a.com
  //              b.com
  // Then verify in a model

  change_processor()->Start();

  RecordsList records;
  records.push_back(SimpleFolderSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "Folder1",
      "1.1.1.1",
      "", true, ""));

  records.push_back(SimpleFolderSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "Folder2",
      "1.1.1.1.1",
      records.at(0)->objectId,
      true, ""));

  records.push_back(SimpleFolderSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "Folder3",
      "1.1.1.1.1.1",
      records.at(1)->objectId,
      true, ""));

  records.push_back(SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "",
      "https://a.com/",
      "A.com - title",
      "1.1.1.1.1.1.1",
      records.at(2)->objectId));

  records.push_back(SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "",
      "https://b.com/",
      "B.com - title",
      "1.1.1.1.1.1.2",
      records.at(2)->objectId));

  change_processor()->ApplyChangesFromSyncModel(records);

  // Verify the model
  ASSERT_EQ(model()->other_node()->child_count(), 1);
  const auto* folder1 = model()->other_node()->GetChild(0);
  EXPECT_EQ(base::UTF16ToUTF8(folder1->GetTitle()), "Folder1");

  ASSERT_EQ(folder1->child_count(), 1);
  const auto* folder2 = folder1->GetChild(0);
  EXPECT_EQ(base::UTF16ToUTF8(folder2->GetTitle()), "Folder2");

  ASSERT_EQ(folder2->child_count(), 1);
  const auto* folder3 = folder2->GetChild(0);
  EXPECT_EQ(base::UTF16ToUTF8(folder3->GetTitle()), "Folder3");

  ASSERT_EQ(folder3->child_count(), 2);
  const auto* node_a = folder3->GetChild(0);
  EXPECT_EQ(node_a->url().spec(), "https://a.com/");
  const auto* node_b = folder3->GetChild(1);
  EXPECT_EQ(node_b->url().spec(), "https://b.com/");
}

TEST_F(BraveBookmarkChangeProcessorTest, ChildrenOfPermanentNodesFromSync) {
  // Record with 1.x.y order, with hideInToolbar=false and empty
  //      parent_object_id should go to toolbar node
  // Record with 2.x.y order, without parent_object_id should go to mobile_node
  // Record with 1.x.y order, with hideInToolbar=true and empty
  //      parent_object_id should go to other_node

  change_processor()->Start();

  RecordsList records;
  records.push_back(SimpleFolderSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "Folder1",
      "1.1.1",
      "", false, ""));

  ASSERT_EQ(model()->bookmark_bar_node()->child_count(), 0);
  change_processor()->ApplyChangesFromSyncModel(records);
  ASSERT_EQ(model()->bookmark_bar_node()->child_count(), 1);
  const auto* folder1 = model()->bookmark_bar_node()->GetChild(0);
  EXPECT_EQ(base::UTF16ToUTF8(folder1->GetTitle()), "Folder1");

  records.clear();
  records.push_back(SimpleFolderSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "Folder2",
      "2.1.1",
      "", false, ""));
  ASSERT_EQ(model()->mobile_node()->child_count(), 0);
  change_processor()->ApplyChangesFromSyncModel(records);
  ASSERT_EQ(model()->mobile_node()->child_count(), 0);
  ASSERT_EQ(model()->bookmark_bar_node()->child_count(), 2);
  const auto* folder2 = model()->bookmark_bar_node()->GetChild(1);
  EXPECT_EQ(base::UTF16ToUTF8(folder2->GetTitle()), "Folder2");

  records.clear();
  records.push_back(SimpleFolderSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "Folder3",
      "1.1.1",
      "", true, ""));
  ASSERT_EQ(model()->other_node()->child_count(), 0);
  change_processor()->ApplyChangesFromSyncModel(records);
  ASSERT_EQ(model()->other_node()->child_count(), 1);
  const auto* folder3 = model()->other_node()->GetChild(0);
  EXPECT_EQ(base::UTF16ToUTF8(folder3->GetTitle()), "Folder3");
}

TEST_F(BraveBookmarkChangeProcessorTest, Utf8FromSync) {
  // Send Greek text
  const wchar_t* const title_wide =
      L"\x03a0\x03b1\x03b3\x03ba\x03cc\x03c3\x03bc\x03b9"
      L"\x03bf\x03c2\x0020\x0399\x03c3\x03c4\x03cc\x03c2";
  auto title_wide_len = std::wcslen(title_wide);

  std::string title_utf8;
  ASSERT_TRUE(base::WideToUTF8(title_wide, title_wide_len, &title_utf8));

  base::string16 title_utf16;
  ASSERT_TRUE(base::WideToUTF16(title_wide, title_wide_len, &title_utf16));

  change_processor()->Start();

  RecordsList records;
  records.push_back(SimpleFolderSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      title_utf8,
      "1.1.1",
      "", false, ""));

  ASSERT_EQ(model()->bookmark_bar_node()->child_count(), 0);
  change_processor()->ApplyChangesFromSyncModel(records);
  const auto* folder1 = model()->bookmark_bar_node()->GetChild(0);
  EXPECT_EQ(folder1->GetTitle(), title_utf16);
}

TEST_F(BraveBookmarkChangeProcessorTest, ChangeOrderUnderSameParentFromSync) {
  BookmarkCreatedFromSyncImpl();

  RecordsList records;
  records.push_back(SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_UPDATE,
      "121, 194, 37, 61, 199, 11, 166, 234, 214, 197, 45, 215, 241, 206, 219, 130",
      "https://a.com/",
      "A.com - title",
      "1.1.1.2", ""));
  records.push_back(SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_UPDATE,
      "",
      "https://b.com/",
      "B.com - title",
      "1.1.1.1", ""));

  change_processor()->ApplyChangesFromSyncModel(records);

  // Expecting we can find the bookmarks in a model
  std::vector<const BookmarkNode*> nodes_a;
  model()->GetNodesByURL(GURL("https://a.com/"), &nodes_a);
  ASSERT_EQ(nodes_a.size(), 1u);
  const auto* node_a = nodes_a.at(0);
  EXPECT_EQ(node_a->url().spec(), "https://a.com/");

  std::vector<const BookmarkNode*> nodes_b;
  model()->GetNodesByURL(GURL("https://b.com/"), &nodes_b);
  ASSERT_EQ(nodes_b.size(), 1u);
  const auto* node_b = nodes_b.at(0);
  EXPECT_EQ(node_b->url().spec(), "https://b.com/");

  EXPECT_EQ(node_a->parent(), node_b->parent());

  int index_a = node_a->parent()->GetIndexOf(node_a);
  EXPECT_NE(index_a, -1);

  int index_b = node_b->parent()->GetIndexOf(node_b);
  EXPECT_NE(index_b, -1);

  EXPECT_LT(index_b, index_a);
}

using brave_sync::jslib::SyncRecord;
::testing::AssertionResult AssertSyncRecordsBookmarkEqual(const char* left_expr,
                                                          const char* right_expr,
                                                          SyncRecord* left,
                                                          SyncRecord* right) {
  DCHECK(left);
  DCHECK(right);

  DCHECK(left->has_bookmark());
  DCHECK(right->has_bookmark());

  #define FAIL_IF_FIELD_NOT_EQUAL(FIELD_NAME)       \
  if (left->FIELD_NAME != right->FIELD_NAME) {    \
    return ::testing::AssertionFailure() << left_expr << " and " << right_expr \
        << " are not equal by " << #FIELD_NAME << " field ("               \
        << left->FIELD_NAME << " vs " << right->FIELD_NAME << ")";    \
  }

  // Ignore action and device_id
  FAIL_IF_FIELD_NOT_EQUAL(objectId);
  FAIL_IF_FIELD_NOT_EQUAL(GetBookmark().site.location);
  FAIL_IF_FIELD_NOT_EQUAL(GetBookmark().site.title);
  FAIL_IF_FIELD_NOT_EQUAL(GetBookmark().isFolder);
  FAIL_IF_FIELD_NOT_EQUAL(GetBookmark().parentFolderObjectId);
  FAIL_IF_FIELD_NOT_EQUAL(GetBookmark().order);
  #undef FAIL_IF_FIELD_NOT_EQUAL

  return ::testing::AssertionSuccess();
}

TEST_F(BraveBookmarkChangeProcessorTest, GetAllSyncData) {
  // This is a resolve operation in terms of sync js lib
  // 1) ApplyChangesFromSyncModel
  // 2) GetAllSyncData() => (must resolve) => SyncRecordAndExistingList
  // 3) Verify all is good

  change_processor()->Start();

  RecordsList records;
  records.push_back(SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "111, 111, 37, 61, 199, 11, 166, 234, 214, 197, 45, 215, 241, 206, 219, 130",
      "https://a.com/",
      "A.com - title",
      "1.1.1.1", ""));
  records.push_back(SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "222, 222, 37, 61, 199, 11, 166, 234, 214, 197, 45, 215, 241, 206, 219, 130",
      "https://b.com/",
      "B.com - title",
      "1.1.1.2", ""));
  records.push_back(SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "33, 33, 37, 61, 199, 11, 166, 234, 214, 197, 45, 215, 241, 206, 219, 130",
      "https://c.com/",
      "C.com - title",
      "1.1.1.3", ""));

  change_processor()->ApplyChangesFromSyncModel(records);

  RecordsList records_to_resolve;

  records_to_resolve.push_back(SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_UPDATE,
      "222, 222, 37, 61, 199, 11, 166, 234, 214, 197, 45, 215, 241, 206, 219, 130",
      "https://b.com/",
      "B.com - title - modified",
      "1.1.1.2", ""));

  records_to_resolve.push_back(SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_DELETE,
      "33, 33, 37, 61, 199, 11, 166, 234, 214, 197, 45, 215, 241, 206, 219, 130",
      "https://c.com/",
      "C.com - title",
      "1.1.1.3", ""));

  records_to_resolve.push_back(SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "44, 44, 37, 61, 199, 11, 166, 234, 214, 197, 45, 215, 241, 206, 219, 130",
      "https://d.com/",
      "D.com - title",
      "1.1.1.4", ""));

  SyncRecordAndExistingList records_and_existing_objects;
  change_processor()->GetAllSyncData(records_to_resolve,
                                                &records_and_existing_objects);
  ASSERT_EQ(records_and_existing_objects.size(), 3u);

  const auto& pair_at_0 = records_and_existing_objects.at(0);

  EXPECT_PRED_FORMAT2(AssertSyncRecordsBookmarkEqual,
      records_to_resolve.at(0).get(), pair_at_0->first.get());
  EXPECT_PRED_FORMAT2(AssertSyncRecordsBookmarkEqual,
       records.at(1).get(), pair_at_0->second.get());

  const auto& pair_at_1 = records_and_existing_objects.at(1);
  EXPECT_PRED_FORMAT2(AssertSyncRecordsBookmarkEqual,
      records_to_resolve.at(1).get(), pair_at_1->first.get());
  EXPECT_PRED_FORMAT2(AssertSyncRecordsBookmarkEqual,
       records.at(2).get(), pair_at_1->second.get());

  const auto& pair_at_2 = records_and_existing_objects.at(2);
  EXPECT_PRED_FORMAT2(AssertSyncRecordsBookmarkEqual,
      records_to_resolve.at(2).get(), pair_at_2->first.get());
  EXPECT_EQ(pair_at_2->second.get(), nullptr);
}

TEST_F(BraveBookmarkChangeProcessorTest, TitleCustomTitle) {
  // Should be able to create folder when title = "" and customTitle != ""
  // Create these:
  // Other
  //    Folder1 (use title)
  //       Folder2 (use customTitle)
  // Then verify in a model

  change_processor()->Start();

  RecordsList records;
  records.push_back(SimpleFolderSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "Folder1",
      "1.1.1.1",
      "", true, ""));

  records.push_back(SimpleFolderSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "",
      "1.1.1.1.1",
      records.at(0)->objectId,
      true,
      "Folder2"));

  change_processor()->ApplyChangesFromSyncModel(records);

  // Verify the model
  ASSERT_EQ(model()->other_node()->child_count(), 1);
  const auto* folder1 = model()->other_node()->GetChild(0);
  EXPECT_EQ(base::UTF16ToUTF8(folder1->GetTitle()), "Folder1");

  ASSERT_EQ(folder1->child_count(), 1);
  const auto* folder2 = folder1->GetChild(0);
  EXPECT_EQ(base::UTF16ToUTF8(folder2->GetTitle()), "Folder2");
}

TEST_F(BraveBookmarkChangeProcessorTest, BookmarkFromMobileGoesToToolbar) {
  change_processor()->Start();
  auto a_record = SimpleBookmarkSyncRecord(
    jslib::SyncRecord::Action::A_CREATE,
    "",
    "https://a.com/",
    "A.com - title",
    "2.1.1",
    "");
  RecordsList records;
  records.push_back(std::move(a_record));
  change_processor()->ApplyChangesFromSyncModel(records);
  // Verify the model, now we should find the folder and the nodes
  EXPECT_EQ(model()->other_node()->child_count(), 0);
  EXPECT_EQ(model()->mobile_node()->child_count(), 0);
  ASSERT_EQ(model()->bookmark_bar_node()->child_count(), 1);

  const auto* node_a = model()->bookmark_bar_node()->GetChild(0);
  EXPECT_EQ(node_a->url().spec(), "https://a.com/");
}

TEST_F(BraveBookmarkChangeProcessorTest, ItemAheadOfFolder) {
  // Create these:
  // Other
  //    Folder1
  //      a.com
  //      b.com
  // With a broken sequence
  // Then verify in a model

  change_processor()->Start();

  auto folder1_record = SimpleFolderSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "Folder1",
      "1.1.1.1.1.1",
      "", true, "");

  auto a_record = SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "",
      "https://a.com/",
      "A.com - title",
      "1.1.1.1.1.1.1",
      folder1_record->objectId);

  auto b_record = SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "",
      "https://b.com/",
      "B.com - title",
      "1.1.1.1.1.1.2",
      folder1_record->objectId);

  RecordsList records;
  records.push_back(std::move(a_record));
  records.push_back(std::move(b_record));

  change_processor()->ApplyChangesFromSyncModel(records);

  // Verify the model, for now we should not find anything
  EXPECT_EQ(model()->GetMostRecentlyAddedUserNodeForURL(GURL("https://a.com/")),
      nullptr);
  EXPECT_EQ(model()->GetMostRecentlyAddedUserNodeForURL(GURL("https://b.com/")),
      nullptr);

  RecordsList records2;
  records2.push_back(std::move(folder1_record));
  change_processor()->ApplyChangesFromSyncModel(records2);

  // Verify the model, now we should find the folder and the nodes
  ASSERT_EQ(model()->other_node()->child_count(), 1);
  const auto* folder1 = model()->other_node()->GetChild(0);
  EXPECT_EQ(base::UTF16ToUTF8(folder1->GetTitle()), "Folder1");

  ASSERT_EQ(folder1->child_count(), 2);
  const auto* node_a = folder1->GetChild(0);
  EXPECT_EQ(node_a->url().spec(), "https://a.com/");
  const auto* node_b = folder1->GetChild(1);
  EXPECT_EQ(node_b->url().spec(), "https://b.com/");
  EXPECT_FALSE(GetPendingNodeRoot()->IsVisible());
}

TEST_F(BraveBookmarkChangeProcessorTest, ItemAheadOfFolderAgressive) {
  // Send these:
  // Other
  //    Folder1
  //       Folder2
  //          Folder3
  //              a.com
  //              b.com
  // In a backwards order:
  //    b.com, a.com,
  //    Folder2,
  //    Folder3,
  //    Folder1
  // Then verify in a model

  change_processor()->Start();

  auto folder1_record = SimpleFolderSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "Folder1",
      "1.1.1.1",
      "", true, "");

  auto folder2_record = SimpleFolderSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "Folder2",
      "1.1.1.1.1",
      folder1_record->objectId,
      true, "");

  auto folder3_record = SimpleFolderSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "Folder3",
      "1.1.1.1.1.1",
      folder2_record->objectId,
      true, "");

  auto a_record = SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "",
      "https://a.com/",
      "A.com - title",
      "1.1.1.1.1.1.1",
      folder3_record->objectId);

  auto b_record = SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "",
      "https://b.com/",
      "B.com - title",
      "1.1.1.1.1.1.2",
      folder3_record->objectId);

  // Send in a backwards order
  {
    RecordsList records1;
    records1.push_back(std::move(b_record));
    records1.push_back(std::move(a_record));
    change_processor()->ApplyChangesFromSyncModel(records1);

    // Verify the model, for now we should not find anything
    EXPECT_EQ(model()->GetMostRecentlyAddedUserNodeForURL(GURL("https://a.com/")),
        nullptr);
    EXPECT_EQ(model()->GetMostRecentlyAddedUserNodeForURL(GURL("https://b.com/")),
        nullptr);
  }

  {
    RecordsList records2;
    records2.push_back(std::move(folder2_record));
    change_processor()->ApplyChangesFromSyncModel(records2);
    EXPECT_EQ(model()->GetMostRecentlyAddedUserNodeForURL(GURL("https://a.com/")),
        nullptr);
    EXPECT_EQ(model()->GetMostRecentlyAddedUserNodeForURL(GURL("https://b.com/")),
        nullptr);
  }

  {
    RecordsList records3;
    records3.push_back(std::move(folder3_record));
    change_processor()->ApplyChangesFromSyncModel(records3);
    EXPECT_EQ(model()->GetMostRecentlyAddedUserNodeForURL(GURL("https://a.com/")),
        nullptr);
    EXPECT_EQ(model()->GetMostRecentlyAddedUserNodeForURL(GURL("https://b.com/")),
        nullptr);
    // TODO(alexeyb): Verify there are some records attached to
    // "Pending Bookmarks" node
  }

  {
    RecordsList records4;
    records4.push_back(std::move(folder1_record));
    change_processor()->ApplyChangesFromSyncModel(records4);
  }

  // Verify now all is as expected
  ASSERT_EQ(model()->other_node()->child_count(), 1);
  const auto* folder1 = model()->other_node()->GetChild(0);
  EXPECT_EQ(base::UTF16ToUTF8(folder1->GetTitle()), "Folder1");

  ASSERT_EQ(folder1->child_count(), 1);
  const auto* folder2 = folder1->GetChild(0);
  EXPECT_EQ(base::UTF16ToUTF8(folder2->GetTitle()), "Folder2");

  ASSERT_EQ(folder2->child_count(), 1);
  const auto* folder3 = folder2->GetChild(0);
  EXPECT_EQ(base::UTF16ToUTF8(folder3->GetTitle()), "Folder3");

  ASSERT_EQ(folder3->child_count(), 2);
  const auto* node_a = folder3->GetChild(0);
  EXPECT_EQ(node_a->url().spec(), "https://a.com/");
  const auto* node_b = folder3->GetChild(1);
  EXPECT_EQ(node_b->url().spec(), "https://b.com/");
  EXPECT_FALSE(GetPendingNodeRoot()->IsVisible());
}

TEST_F(BraveBookmarkChangeProcessorTest, ItemAheadOfFolderRequireStrictSorting) {
  // Send these:
  // Other
  // +--0-1.com              1.1.1.1
  // +--Folder1              1.1.1.2
  // |  +--1-1.com           1.1.1.2.1
  // |  +--Folder2           1.1.1.2.2
  // |  |  +--2-1.com        1.1.1.2.2.1
  // |  |  +--Folder3        1.1.1.2.2.2
  // |  |  |  +--a.com       1.1.1.2.2.2.1
  // |  |  |  +--b.com       1.1.1.2.2.2.2
  // |  |  +--2-2.com        1.1.1.2.2.3
  // |  |  +--2-3.com        1.1.1.2.2.4
  // |  +--1-2.com           1.1.1.2.3
  // +--0-2.com              1.1.1.3
  //
  // In an order:
  //    Folder2, 2-1.com, Folder3, a.com, b.com
  //    2-3.com, 2-2.com, 1-2.com
  //    Folder1, 0-1.com, 0-2.com, 1-1.com
  // Then verify in a model

  change_processor()->Start();

  auto folder1_record = SimpleFolderSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "Folder1",
      "1.1.1.2",
      "", true, "");

  auto folder2_record = SimpleFolderSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "Folder2",
      "1.1.1.2.2",
      folder1_record->objectId,
      true, "");

  auto folder3_record = SimpleFolderSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "Folder3",
      "1.1.1.2.2.2",
      folder2_record->objectId,
      true, "");

  auto _0_1_record = SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "",
      "https://0-1.com/",
      "0-1.com - title",
      "1.1.1.1",
      "");
  auto _0_2_record = SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "",
      "https://0-2.com/",
      "0-2.com - title",
      "1.1.1.3",
      "");

  auto _1_1_record = SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "",
      "https://1-1.com/",
      "1-1.com - title",
      "1.1.1.2.1",
      folder1_record->objectId);
  auto _1_2_record = SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "",
      "https://1-2.com/",
      "1-2.com - title",
      "1.1.1.2.3",
      folder1_record->objectId);

  auto _2_1_record = SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "",
      "https://2-1.com/",
      "2-1.com - title",
      "1.1.1.2.2.1",
      folder2_record->objectId);
  auto _2_2_record = SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "",
      "https://2-2.com/",
      "2-2.com - title",
      "1.1.1.2.2.3",
      folder2_record->objectId);
  auto _2_3_record = SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "",
      "https://2-3.com/",
      "2-3.com - title",
      "1.1.1.2.2.4",
      folder2_record->objectId);

  auto a_record = SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "",
      "https://a.com/",
      "A.com - title",
      "1.1.1.1.1.1.1",
      folder3_record->objectId);

  auto b_record = SimpleBookmarkSyncRecord(
      jslib::SyncRecord::Action::A_CREATE,
      "",
      "https://b.com/",
      "B.com - title",
      "1.1.1.1.1.1.2",
      folder3_record->objectId);

  // Send in an order
  {
    RecordsList records1;
    records1.push_back(std::move(folder2_record));
    records1.push_back(std::move(_2_1_record));
    records1.push_back(std::move(folder3_record));
    records1.push_back(std::move(a_record));
    records1.push_back(std::move(b_record));
    change_processor()->ApplyChangesFromSyncModel(records1);
  }

  {
    RecordsList records2;
    // 2-3 before 2-2 is important
    records2.push_back(std::move(_2_3_record));
    records2.push_back(std::move(_2_2_record));
    records2.push_back(std::move(_1_2_record));
    change_processor()->ApplyChangesFromSyncModel(records2);
  }

  {
    RecordsList records3;
    records3.push_back(std::move(folder1_record));
    records3.push_back(std::move(_0_1_record));
    records3.push_back(std::move(_0_2_record));
    records3.push_back(std::move(_1_1_record));

    change_processor()->ApplyChangesFromSyncModel(records3);
  }

  // Verify now all is as expected
  ASSERT_EQ(model()->other_node()->child_count(), 3);
  const auto* node_0_1 = model()->other_node()->GetChild(0);
  EXPECT_EQ(node_0_1->url().spec(), "https://0-1.com/");
  const auto* folder1 = model()->other_node()->GetChild(1);
  EXPECT_EQ(base::UTF16ToUTF8(folder1->GetTitle()), "Folder1");
  const auto* node_0_2 = model()->other_node()->GetChild(2);
  EXPECT_EQ(node_0_2->url().spec(), "https://0-2.com/");

  ASSERT_EQ(folder1->child_count(), 3);
  const auto* node_1_1 = folder1->GetChild(0);
  EXPECT_EQ(node_1_1->url().spec(), "https://1-1.com/");
  const auto* folder2 = folder1->GetChild(1);
  EXPECT_EQ(base::UTF16ToUTF8(folder2->GetTitle()), "Folder2");
  const auto* node_1_2 = folder1->GetChild(2);
  EXPECT_EQ(node_1_2->url().spec(), "https://1-2.com/");

  ASSERT_EQ(folder2->child_count(), 4);
  const auto* node_2_1 = folder2->GetChild(0);
  EXPECT_EQ(node_2_1->url().spec(), "https://2-1.com/");
  const auto* folder3 = folder2->GetChild(1);
  EXPECT_EQ(base::UTF16ToUTF8(folder3->GetTitle()), "Folder3");
  // Below fails if GetIndex uses tree iteartor
  const auto* node_2_2 = folder2->GetChild(2);
  EXPECT_EQ(node_2_2->url().spec(), "https://2-2.com/");

  ASSERT_EQ(folder3->child_count(), 2);
  const auto* node_a = folder3->GetChild(0);
  EXPECT_EQ(node_a->url().spec(), "https://a.com/");
  const auto* node_b = folder3->GetChild(1);
  EXPECT_EQ(node_b->url().spec(), "https://b.com/");
  EXPECT_FALSE(GetPendingNodeRoot()->IsVisible());
}

TEST_F(BraveBookmarkChangeProcessorTest, IgnoreRapidCreateDelete) {
  change_processor()->Start();

  const auto* node_a = model()->AddURL(model()->other_node(), 0,
                           base::ASCIIToUTF16("A.com - title"),
                           GURL("https://a.com/"));
  model()->Remove(node_a);

  EXPECT_EQ(change_processor()->GetDeletedNodeRoot()->child_count(), 0);

  // Expect there will be no calls, because no any records to send
  EXPECT_CALL(*sync_client(), SendSyncRecords("BOOKMARKS", _)).Times(0);
  change_processor()->SendUnsynced(base::TimeDelta::FromMinutes(10));
}

TEST_F(BraveBookmarkChangeProcessorTest, IgnoreMetadataSet) {
  change_processor()->Start();

  const auto* node_a = model()->AddURL(model()->other_node(), 0,
                           base::ASCIIToUTF16("A.com - title"),
                           GURL("https://a.com/"));

  EXPECT_CALL(*sync_client(), SendSyncRecords("BOOKMARKS", _)).Times(1);
  change_processor()->SendUnsynced(base::TimeDelta::FromMinutes(10));

  model()->SetNodeMetaInfo(node_a, "last_visited", "2019");
  // Not interested in metadata changes, expecting no calls
  EXPECT_CALL(*sync_client(), SendSyncRecords("BOOKMARKS", _)).Times(0);
  change_processor()->SendUnsynced(base::TimeDelta::FromMinutes(10));
}
