/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/guid.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_sync/syncer_helper.h"
#include "brave/components/brave_sync/test_util.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/bookmarks/test/test_bookmark_client.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::AtLeast;

using bookmarks::BookmarkClient;
using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

namespace brave_sync {

namespace {

// |node| is near the end in parent
void RepositionRespectOrder(bookmarks::BookmarkModel* bookmark_model,
                            const bookmarks::BookmarkNode* node) {
  const bookmarks::BookmarkNode* parent = node->parent();
  int index = GetIndex(parent, node);
  bookmark_model->Move(node, parent, index);
}

}  // namespace

class SyncerHelperTest : public testing::Test {
 public:
  SyncerHelperTest() {}
  ~SyncerHelperTest() override {}

 protected:
  void SetUp() override {
    EXPECT_TRUE(temp_dir_.CreateUniqueTempDir());

    profile_ = CreateBraveSyncProfile(temp_dir_.GetPath());
    EXPECT_TRUE(profile_.get() != NULL);

    BookmarkModelFactory::GetInstance()->SetTestingFactory(
        profile_.get(), base::BindRepeating(&BuildFakeBookmarkModelForTests));

    model_ = BookmarkModelFactory::GetForBrowserContext(
        Profile::FromBrowserContext(profile_.get()));

    EXPECT_NE(bookmark_client(), nullptr);
    EXPECT_NE(model(), nullptr);

    // SetPermanentNodesOrder
    model_->SetNodeMetaInfo(model_->bookmark_bar_node(), "order", "1.0.1");

    model_->SetNodeMetaInfo(model_->other_node(), "order", "1.0.2");
  }

  void TearDown() override { profile_.reset(); }

  BookmarkClient* bookmark_client() { return model_->client(); }
  BookmarkModel* model() { return model_; }

 private:
  // Need this as a very first member to run tests in UI thread
  // When this is set, class should not install any other MessageLoops, like
  // base::test::ScopedTaskEnvironment
  content::BrowserTaskEnvironment task_environment_;

  BookmarkModel* model_;  // Not owns
  std::unique_ptr<Profile> profile_;
  base::ScopedTempDir temp_dir_;
};

TEST_F(SyncerHelperTest, AddBraveMetaInfoCreateOrUpdate) {
  std::string order;
  std::string sync_timestamp;
  const auto* folder1 = model()->AddFolder(model()->bookmark_bar_node(), 0,
                                           base::ASCIIToUTF16("Folder1"));
  AddBraveMetaInfo(folder1);
  folder1->GetMetaInfo("order", &order);
  EXPECT_EQ(order, "1.0.1.1");
  std::string folder1_id;
  folder1->GetMetaInfo("object_id", &folder1_id);
  EXPECT_TRUE(!folder1_id.empty());
  std::string folder1_parent_id;
  folder1->GetMetaInfo("parent_object_id", &folder1_parent_id);
  EXPECT_TRUE(folder1_parent_id.empty());
  folder1->GetMetaInfo("sync_timestamp", &sync_timestamp);
  EXPECT_TRUE(!sync_timestamp.empty());

  const auto* node_a = model()->AddURL(
      folder1, 0, base::ASCIIToUTF16("A.com - title"), GURL("https://a.com/"));
  order.clear();
  sync_timestamp.clear();
  AddBraveMetaInfo(node_a);
  node_a->GetMetaInfo("order", &order);
  EXPECT_EQ(order, "1.0.1.1.1");
  std::string node_a_id;
  node_a->GetMetaInfo("object_id", &node_a_id);
  EXPECT_TRUE(!node_a_id.empty());
  std::string node_a_parent_id;
  node_a->GetMetaInfo("parent_object_id", &node_a_parent_id);
  EXPECT_EQ(node_a_parent_id, folder1_id);
  node_a->GetMetaInfo("sync_timestamp", &sync_timestamp);
  EXPECT_TRUE(!sync_timestamp.empty());

  // Update
  order.clear();
  node_a_id.clear();
  node_a_parent_id.clear();
  model()->SetURL(node_a, GURL("https://a-m.com/"));
  AddBraveMetaInfo(node_a);
  node_a->GetMetaInfo("order", &order);
  EXPECT_EQ(order, "1.0.1.1.1");
  node_a->GetMetaInfo("object_id", &node_a_id);
  EXPECT_TRUE(!node_a_id.empty());
  node_a->GetMetaInfo("parent_object_id", &node_a_parent_id);
  EXPECT_EQ(node_a_parent_id, folder1_id);
  std::string new_sync_timestamp;
  node_a->GetMetaInfo("sync_timestamp", &new_sync_timestamp);
  EXPECT_EQ(new_sync_timestamp, sync_timestamp);
}

TEST_F(SyncerHelperTest, AddBraveMetaInfoNodeMoved) {
  const auto* folder1 = model()->AddFolder(model()->bookmark_bar_node(), 0,
                                           base::ASCIIToUTF16("Folder1"));
  AddBraveMetaInfo(folder1);
  const auto* node_a = model()->AddURL(
      folder1, 0, base::ASCIIToUTF16("A.com - title"), GURL("https://a.com/"));
  AddBraveMetaInfo(node_a);
  model()->Move(node_a, model()->bookmark_bar_node(), 1);
  AddBraveMetaInfo(node_a);

  std::string order;
  node_a->GetMetaInfo("order", &order);
  EXPECT_EQ(order, "1.0.1.2");
  std::string node_a_id;
  node_a->GetMetaInfo("object_id", &node_a_id);
  EXPECT_TRUE(!node_a_id.empty());
  std::string node_a_parent_id;
  node_a->GetMetaInfo("parent_object_id", &node_a_parent_id);
  EXPECT_TRUE(node_a_parent_id.empty());
  std::string sync_timestamp;
  node_a->GetMetaInfo("sync_timestamp", &sync_timestamp);
  EXPECT_TRUE(!sync_timestamp.empty());
}

TEST_F(SyncerHelperTest, AddBraveMetaInfoNodeChildrenReordered) {
  const auto* node_a = model()->AddURL(model()->bookmark_bar_node(), 0,
                                       base::ASCIIToUTF16("A.com - title"),
                                       GURL("https://a.com/"));
  AddBraveMetaInfo(node_a);
  const auto* node_b = model()->AddURL(model()->bookmark_bar_node(), 1,
                                       base::ASCIIToUTF16("B.com - title"),
                                       GURL("https://b.com/"));
  AddBraveMetaInfo(node_b);
  const auto* node_c = model()->AddURL(model()->bookmark_bar_node(), 2,
                                       base::ASCIIToUTF16("C.com - title"),
                                       GURL("https://c.com/"));
  AddBraveMetaInfo(node_c);

  // Expecting to have initially:
  // 'Bookmarks Bar'   1.0.1
  //  |-A.com          1.0.1.1
  //  |-B.com          1.0.1.2
  //  |-C.com          1.0.1.3

  std::string order_a;
  std::string order_b;
  std::string order_c;
  node_a->GetMetaInfo("order", &order_a);
  EXPECT_EQ(order_a, "1.0.1.1");
  node_b->GetMetaInfo("order", &order_b);
  EXPECT_EQ(order_b, "1.0.1.2");
  node_c->GetMetaInfo("order", &order_c);
  EXPECT_EQ(order_c, "1.0.1.3");

  model()->Move(node_c, model()->bookmark_bar_node(), 0);
  AddBraveMetaInfo(node_c);

  // After move to have:
  // 'Bookmarks Bar'   1.0.1
  //  |-C.com          1.0.1.0.1
  //  |-A.com          1.0.1.1
  //  |-B.com          1.0.1.2

  order_a.clear();
  order_b.clear();
  order_c.clear();

  node_a->GetMetaInfo("order", &order_a);
  EXPECT_EQ(order_a, "1.0.1.1");
  node_b->GetMetaInfo("order", &order_b);
  EXPECT_EQ(order_b, "1.0.1.2");
  node_c->GetMetaInfo("order", &order_c);
  EXPECT_EQ(order_c, "1.0.1.0.1");
}

TEST_F(SyncerHelperTest, AddBraveMetaInfoNodeMovedReordered) {
  const auto* node_a = model()->AddURL(model()->bookmark_bar_node(), 0,
                                       base::ASCIIToUTF16("A.com - title"),
                                       GURL("https://a.com/"));
  AddBraveMetaInfo(node_a);
  const auto* folder1 = model()->AddFolder(model()->bookmark_bar_node(), 1,
                                           base::ASCIIToUTF16("Folder1"));
  AddBraveMetaInfo(folder1);
  const auto* node_b = model()->AddURL(
      folder1, 0, base::ASCIIToUTF16("B.com - title"), GURL("https://b.com/"));
  AddBraveMetaInfo(node_b);
  const auto* node_c = model()->AddURL(
      folder1, 1, base::ASCIIToUTF16("C.com - title"), GURL("https://c.com/"));
  AddBraveMetaInfo(node_c);

  // Expecting here to have:
  // 'Bookmarks Bar'   1.0.1
  //  |-A.com          1.0.1.1
  //  |-Folder1        1.0.1.2
  //    |-B.com        1.0.1.2.1
  //    |-C.com        1.0.1.2.2

  std::string order_a;
  std::string order_b;
  std::string order_c;
  std::string order_folder1;
  node_a->GetMetaInfo("order", &order_a);
  EXPECT_EQ(order_a, "1.0.1.1");
  node_b->GetMetaInfo("order", &order_b);
  EXPECT_EQ(order_b, "1.0.1.2.1");
  node_c->GetMetaInfo("order", &order_c);
  EXPECT_EQ(order_c, "1.0.1.2.2");
  folder1->GetMetaInfo("order", &order_folder1);
  EXPECT_EQ(order_folder1, "1.0.1.2");

  model()->Move(node_a, folder1, 0);
  AddBraveMetaInfo(node_a);

  order_a.clear();
  order_b.clear();
  order_c.clear();
  order_folder1.clear();

  // After move expecting have:
  // 'Bookmarks Bar'   1.0.1       (kept)
  //  |-Folder1        1.0.1.2     (kept)
  //    |-A.com        1.0.1.2.0.1 (re-calculated)
  //    |-B.com        1.0.1.2.1   (kept)
  //    |-C.com        1.0.1.2.2   (kept)

  node_a->GetMetaInfo("order", &order_a);
  EXPECT_EQ(order_a, "1.0.1.2.0.1");
  node_b->GetMetaInfo("order", &order_b);
  EXPECT_EQ(order_b, "1.0.1.2.1");
  node_c->GetMetaInfo("order", &order_c);
  EXPECT_EQ(order_c, "1.0.1.2.2");
  folder1->GetMetaInfo("order", &order_folder1);
  EXPECT_EQ(order_folder1, "1.0.1.2");
}

TEST_F(SyncerHelperTest, GetIndexInPermanentNodes) {
  BookmarkNode node(/*id=*/0, base::GenerateGUID(), GURL("https://brave.com"));
  node.SetMetaInfo("object_id", "notused");
  node.SetMetaInfo("order", "1.0.1.1");
  EXPECT_EQ(GetIndex(model()->bookmark_bar_node(), &node), 0u);

  node.SetMetaInfo("order", "1.0.2.1");
  EXPECT_EQ(GetIndex(model()->other_node(), &node), 0u);

  const auto* node_a =
      model()->AddURL(model()->bookmark_bar_node(), 0,
                      base::ASCIIToUTF16("a.com"), GURL("https://a.com/"));
  model()->SetNodeMetaInfo(node_a, "object_id", "notused");
  // compare device id
  model()->SetNodeMetaInfo(node_a, "order", "1.1.1.1");
  node.SetMetaInfo("order", "1.0.1.1");
  EXPECT_EQ(GetIndex(model()->bookmark_bar_node(), &node), 0u);
  model()->SetNodeMetaInfo(node_a, "order", "1.0.1.1");
  node.SetMetaInfo("order", "1.1.1.1");
  EXPECT_EQ(GetIndex(model()->bookmark_bar_node(), &node), 1u);

  // compare platform id
  model()->SetNodeMetaInfo(node_a, "order", "2.0.1.1");
  node.SetMetaInfo("order", "1.0.1.1");
  EXPECT_EQ(GetIndex(model()->bookmark_bar_node(), &node), 0u);
  model()->SetNodeMetaInfo(node_a, "order", "1.0.1.1");
  node.SetMetaInfo("order", "2.0.1.1");
  EXPECT_EQ(GetIndex(model()->bookmark_bar_node(), &node), 1u);
}

TEST_F(SyncerHelperTest, GetIndexMoreChildren) {
  for (int i = 0; i < 10; ++i) {
    const auto* node_a =
        model()->AddURL(model()->bookmark_bar_node(), i,
                        base::ASCIIToUTF16("a.com"), GURL("https://a.com/"));
    std::string order = "1.1.1." + base::NumberToString(i == 9 ? i + 2 : i + 1);
    model()->SetNodeMetaInfo(node_a, "order", order);
    model()->SetNodeMetaInfo(node_a, "object_id", "notused");
  }
  // inserted as first child
  BookmarkNode node(/*id=*/9, base::GenerateGUID(), GURL("https://brave.com"));
  node.SetMetaInfo("object_id", "notused");
  node.SetMetaInfo("order", "1.0.1.10");
  EXPECT_EQ(GetIndex(model()->bookmark_bar_node(), &node), 0u);
  // inserted as 10th child
  node.SetMetaInfo("order", "1.1.1.10");
  EXPECT_EQ(GetIndex(model()->bookmark_bar_node(), &node), 9u);
}

TEST_F(SyncerHelperTest, GetIndexInFolder) {
  const auto* folder1 = model()->AddFolder(model()->bookmark_bar_node(), 0,
                                           base::ASCIIToUTF16("Folder1"));
  model()->SetNodeMetaInfo(folder1, "order", "1.0.1.1");
  BookmarkNode node(/*id=*/1, base::GenerateGUID(), GURL("https://brave.com"));
  node.SetMetaInfo("object_id", "notused");
  node.SetMetaInfo("order", "1.0.1.1.1");
  EXPECT_EQ(GetIndex(folder1, &node), 0u);

  // appended at the end
  const auto* node_a = model()->AddURL(folder1, 0, base::ASCIIToUTF16("a.com"),
                                       GURL("https://a.com/"));
  model()->SetNodeMetaInfo(node_a, "order", "1.0.1.1.1");
  model()->SetNodeMetaInfo(node_a, "object_id", "notused");
  node.SetMetaInfo("order", "1.0.1.1.2");
  EXPECT_EQ(GetIndex(folder1, &node), 1u);
}

TEST_F(SyncerHelperTest, SameOrderBookmarksSordetByObjectIdFull3) {
  // This test emulates folowing STR
  // 1. on device A create bookmarks A1.com and A2.com
  // 2. on device B create bookmarks B1.com and B2.com
  // 3. create sync chain on device A and connect device B with a codephrase
  // 4. wait for bookmarks will be synchronized between device A and B
  // 5. on device A in Add bookmark dialog enter Name A3.com, URL A3.com,
  //    but dont press Save button
  // 6. repeat pt 5 on device B, for B3.com
  // 7. press Save button on devices A and B
  // Expected bookmarks on devices A and B are sorted in the same way
  const auto* node_a1 =
      model()->AddURL(model()->bookmark_bar_node(), 0,
                      base::ASCIIToUTF16("A1.com"), GURL("https://a1.com/"));
  AddBraveMetaInfo(node_a1);
  const auto* node_a2 =
      model()->AddURL(model()->bookmark_bar_node(), 1,
                      base::ASCIIToUTF16("A2.com"), GURL("https://a2.com/"));
  AddBraveMetaInfo(node_a2);
  const auto* node_b1 =
      model()->AddURL(model()->bookmark_bar_node(), 2,
                      base::ASCIIToUTF16("B1.com"), GURL("https://b1.com/"));
  AddBraveMetaInfo(node_b1);
  const auto* node_b2 =
      model()->AddURL(model()->bookmark_bar_node(), 3,
                      base::ASCIIToUTF16("B2.com"), GURL("https://b2.com/"));
  AddBraveMetaInfo(node_b2);

  // Expect b1 and b2 no need to move
  uint64_t index_to_move_b1 =
    GetIndex(model()->bookmark_bar_node(), node_b1);
  // Expecting index for Move operation 3, after Move position of node will be 2
  EXPECT_EQ(index_to_move_b1, 3u);
  RepositionRespectOrder(model(), node_b1);
  auto title_at_2 = model()->bookmark_bar_node()->children()[2]->GetTitle();
  EXPECT_EQ(title_at_2, base::ASCIIToUTF16("B1.com"));

  uint64_t index_to_move_b2 =
    GetIndex(model()->bookmark_bar_node(), node_b2);
  // Expecting index for Move operation 4, after Move position of node will be 3
  EXPECT_EQ(index_to_move_b2, 4u);
  RepositionRespectOrder(model(), node_b2);
  auto title_at_3 = model()->bookmark_bar_node()->children()[3]->GetTitle();
  EXPECT_EQ(title_at_3, base::ASCIIToUTF16("B2.com"));

  const auto* node_a3 =
      model()->AddURL(model()->bookmark_bar_node(), 4,
                      base::ASCIIToUTF16("A3.com"), GURL("https://a3.com/"));
  AddBraveMetaInfo(node_a3);
  const auto* node_b3 =
      model()->AddURL(model()->bookmark_bar_node(), 5,
                      base::ASCIIToUTF16("B3.com"), GURL("https://b3.com/"));
  AddBraveMetaInfo(node_b3);
  const auto* node_c3 =
      model()->AddURL(model()->bookmark_bar_node(), 6,
                      base::ASCIIToUTF16("C3.com"), GURL("https://c3.com/"));
  AddBraveMetaInfo(node_c3);

  std::string a3_order;
  node_a3->GetMetaInfo("order", &a3_order);
  EXPECT_TRUE(!a3_order.empty());

  std::string a3_object_id;
  node_a3->GetMetaInfo("object_id", &a3_object_id);
  EXPECT_TRUE(!a3_object_id.empty());

  // Emulating nodes a3, b3, and c3 have the same order
  const_cast<BookmarkNode*>(node_b3)->SetMetaInfo("order", a3_order);
  const_cast<BookmarkNode*>(node_c3)->SetMetaInfo("order", a3_order);

  // Expecting sorting of same order bookmarks by object_id
  // object_id is 16 comma and spaces separated values of 16 uint8
  // Will assign these object ids to make RepositionRespectOrder do sorting:
  //  C3      A3       B3
  // "..." < 1,2,3 < "@@@"
  ASSERT_TRUE("..." < a3_object_id && a3_object_id < "@@@");
  const_cast<BookmarkNode*>(node_b3)->SetMetaInfo("object_id", "@@@");
  const_cast<BookmarkNode*>(node_c3)->SetMetaInfo("object_id", "...");

  //  0  1  2  3       4        5        6
  // A1 A2 B1 B2  A3(1,2,3)  B3(@@@)  C3(...)
  auto title_at_4 = model()->bookmark_bar_node()->children()[4]->GetTitle();
  EXPECT_EQ(title_at_4, base::ASCIIToUTF16("A3.com"));
  auto title_at_5 = model()->bookmark_bar_node()->children()[5]->GetTitle();
  EXPECT_EQ(title_at_5, base::ASCIIToUTF16("B3.com"));
  auto title_at_6 = model()->bookmark_bar_node()->children()[6]->GetTitle();
  EXPECT_EQ(title_at_6, base::ASCIIToUTF16("C3.com"));

  RepositionRespectOrder(model(), node_b3);
  //  0  1  2  3       4        5        6
  // A1 A2 B1 B2  A3(1,2,3)  C3(...)  B3(@@@)
  // node B3 gone after C3 because "..." < "@@@"
  title_at_4 = model()->bookmark_bar_node()->children()[4]->GetTitle();
  EXPECT_EQ(title_at_4, base::ASCIIToUTF16("A3.com"));
  title_at_5 = model()->bookmark_bar_node()->children()[5]->GetTitle();
  EXPECT_EQ(title_at_5, base::ASCIIToUTF16("C3.com"));
  title_at_6 = model()->bookmark_bar_node()->children()[6]->GetTitle();
  EXPECT_EQ(title_at_6, base::ASCIIToUTF16("B3.com"));

  RepositionRespectOrder(model(), node_c3);
  //  0  1  2  3     4        5        6
  // A1 A2 B1 B2  C3(...) A3(1,2,3) B3(@@@)
  // node C3 moved to the correct position

  title_at_4 = model()->bookmark_bar_node()->children()[4]->GetTitle();
  EXPECT_EQ(title_at_4, base::ASCIIToUTF16("C3.com"));
  title_at_5 = model()->bookmark_bar_node()->children()[5]->GetTitle();
  EXPECT_EQ(title_at_5, base::ASCIIToUTF16("A3.com"));
  title_at_6 = model()->bookmark_bar_node()->children()[6]->GetTitle();
  EXPECT_EQ(title_at_6, base::ASCIIToUTF16("B3.com"));
}

TEST_F(SyncerHelperTest, RemoteUpdatesHandlerNeedsMoveBookmark) {
  // Emulating such STR
  // 1) Device A
  // [0] interia.pl     1.0.1.1
  // [1] youtube.com    1.0.1.2
  // 2) Device B does change order:
  // [0] youtube.com    1.0.1.2
  // [1] interia.pl     1.0.1.3
  // 3) Device A falls into
  // [0] interia.pl     1.0.1.3
  // [1] youtube.com    1.0.1.2
  // 4) Device A wants to know a new index for Move operation on interia.pl,
  // which is expected to be 2

  const auto* node_interia_pl = model()->AddURL(
      model()->bookmark_bar_node(), 0, base::ASCIIToUTF16("interia.pl"),
      GURL("https://interia.pl/"));
  AddBraveMetaInfo(node_interia_pl);
  const auto* node_youtube_com = model()->AddURL(
      model()->bookmark_bar_node(), 1, base::ASCIIToUTF16("youtube.com"),
      GURL("https://youtube.com/"));
  AddBraveMetaInfo(node_youtube_com);

  std::string interia_pl_order;
  node_interia_pl->GetMetaInfo("order", &interia_pl_order);
  EXPECT_EQ(interia_pl_order, "1.0.1.1");

  std::string youtube_com_order;
  node_youtube_com->GetMetaInfo("order", &youtube_com_order);
  EXPECT_EQ(youtube_com_order, "1.0.1.2");

  const_cast<BookmarkNode*>(node_interia_pl)->SetMetaInfo("order", "1.0.1.3");
  int index = GetIndex(model()->bookmark_bar_node(), node_interia_pl);
  EXPECT_EQ(index, 2);

  RepositionRespectOrder(model(), node_interia_pl);
  auto title_at_1 = model()->bookmark_bar_node()->children()[1]->GetTitle();
  EXPECT_EQ(title_at_1, base::ASCIIToUTF16("interia.pl"));
}

TEST_F(SyncerHelperTest, RemoteUpdatesHandlerKeepBookmarkPosition) {
  // Emulating such STR
  // 1) Device A
  // [0] interia.pl     1.0.1.1
  // [1] youtube.com    1.0.1.2
  // 2) Device B doesn't change order
  // 3) Device A falls into
  // [0] interia.pl     1.0.1.1
  // [1] youtube.com    1.0.1.2
  // 4) Device A wants to know a new index for Move operation on interia.pl,
  // which is expected to be 1, and after Move operation interia.pl will be on
  // the position 0

  const auto* node_interia_pl = model()->AddURL(
      model()->bookmark_bar_node(), 0, base::ASCIIToUTF16("interia.pl"),
      GURL("https://interia.pl/"));
  AddBraveMetaInfo(node_interia_pl);
  const auto* node_youtube_com = model()->AddURL(
      model()->bookmark_bar_node(), 1, base::ASCIIToUTF16("youtube.com"),
      GURL("https://youtube.com/"));
  AddBraveMetaInfo(node_youtube_com);

  std::string interia_pl_order;
  node_interia_pl->GetMetaInfo("order", &interia_pl_order);
  EXPECT_EQ(interia_pl_order, "1.0.1.1");

  std::string youtube_com_order;
  node_youtube_com->GetMetaInfo("order", &youtube_com_order);
  EXPECT_EQ(youtube_com_order, "1.0.1.2");

  int index = GetIndex(model()->bookmark_bar_node(), node_interia_pl);
  // Expecting index for Move operation 1, after Move position of node will be 0
  EXPECT_EQ(index, 1);

  RepositionRespectOrder(model(), node_interia_pl);
  auto title_at_0 = model()->bookmark_bar_node()->children()[0]->GetTitle();
  EXPECT_EQ(title_at_0, base::ASCIIToUTF16("interia.pl"));
}

TEST_F(SyncerHelperTest, RemoteUpdatesHandlerBookmarkPositionAmong4) {
  // Emulating STR we need to move bookmark in the middle
  // 1) Device A
  // [0] A.com     1.0.1.1
  // [1] B.com     1.0.1.2
  // [2] C.com     1.0.1.3
  // [3] D.com     1.0.1.4

  // 2) Device B does change order:
  // [0] A.com     1.0.1.1
  // [1] C.com     1.0.1.3
  // [2] B.com     1.0.1.3.1
  // [3] D.com     1.0.1.4

  // 3) Device A falls into
  // [0] A.com     1.0.1.1
  // [1] B.com     1.0.1.3.1
  // [2] C.com     1.0.1.3
  // [3] D.com     1.0.1.4

  // 4) Device A wants to know a new index for Move on node B.com,
  // which is expected to be 3 counting the target node is still in original
  // position, it will be 2 after move

  // 5) After move we have on device A
  // [0] A.com     1.0.1.1
  // [1] C.com     1.0.1.3
  // [2] B.com     1.0.1.3.1
  // [3] D.com     1.0.1.4

  const auto* node_a =
      model()->AddURL(model()->bookmark_bar_node(), 0,
                      base::ASCIIToUTF16("A.com"), GURL("https://a.com/"));
  AddBraveMetaInfo(node_a);
  const auto* node_b =
      model()->AddURL(model()->bookmark_bar_node(), 1,
                      base::ASCIIToUTF16("B.com"), GURL("https://b.com/"));
  AddBraveMetaInfo(node_b);
  const auto* node_c =
      model()->AddURL(model()->bookmark_bar_node(), 2,
                      base::ASCIIToUTF16("C.com"), GURL("https://c.com/"));
  AddBraveMetaInfo(node_c);
  const auto* node_d =
      model()->AddURL(model()->bookmark_bar_node(), 3,
                      base::ASCIIToUTF16("D.com"), GURL("https://d.com/"));
  AddBraveMetaInfo(node_d);

  std::string a_order;
  node_a->GetMetaInfo("order", &a_order);
  EXPECT_TRUE(!a_order.empty());
  EXPECT_EQ(a_order, "1.0.1.1");

  std::string b_order;
  node_b->GetMetaInfo("order", &b_order);
  EXPECT_TRUE(!b_order.empty());
  EXPECT_EQ(b_order, "1.0.1.2");

  std::string c_order;
  node_c->GetMetaInfo("order", &c_order);
  EXPECT_TRUE(!b_order.empty());
  EXPECT_EQ(c_order, "1.0.1.3");

  std::string d_order;
  node_d->GetMetaInfo("order", &d_order);
  EXPECT_TRUE(!d_order.empty());
  EXPECT_EQ(d_order, "1.0.1.4");

  const_cast<BookmarkNode*>(node_b)->SetMetaInfo("order", "1.0.1.3.1");
  int index = GetIndex(model()->bookmark_bar_node(), node_b);
  // Expecting index for Move operation 3, after Move position of node will be 2
  EXPECT_EQ(index, 3);

  RepositionRespectOrder(model(), node_b);

  auto title_at_0 = model()->bookmark_bar_node()->children()[0]->GetTitle();
  EXPECT_EQ(title_at_0, base::ASCIIToUTF16("A.com"));
  auto title_at_1 = model()->bookmark_bar_node()->children()[1]->GetTitle();
  EXPECT_EQ(title_at_1, base::ASCIIToUTF16("C.com"));
  auto title_at_2 = model()->bookmark_bar_node()->children()[2]->GetTitle();
  EXPECT_EQ(title_at_2, base::ASCIIToUTF16("B.com"));
  auto title_at_3 = model()->bookmark_bar_node()->children()[3]->GetTitle();
  EXPECT_EQ(title_at_3, base::ASCIIToUTF16("D.com"));
}

}  // namespace brave_sync
