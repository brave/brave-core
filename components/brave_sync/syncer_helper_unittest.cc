/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <memory>
#include <string>
#include <vector>

#include "base/files/scoped_temp_dir.h"
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
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::AtLeast;

using bookmarks::BookmarkClient;
using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

namespace brave_sync {

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
       profile_.get(),
       base::BindRepeating(&BuildFakeBookmarkModelForTests));

    model_ = BookmarkModelFactory::GetForBrowserContext(
        Profile::FromBrowserContext(profile_.get()));

    EXPECT_NE(bookmark_client(), nullptr);
    EXPECT_NE(model(), nullptr);

    // SetPermanentNodesOrder
    model_->SetNodeMetaInfo(model_->bookmark_bar_node(), "order", "1.0.1");

    model_->SetNodeMetaInfo(model_->other_node(), "order", "1.0.2");

  }

  void TearDown() override {
    profile_.reset();
  }

  BookmarkClient* bookmark_client() { return model_->client(); }
  BookmarkModel* model() { return model_; }

 private:
  // Need this as a very first member to run tests in UI thread
  // When this is set, class should not install any other MessageLoops, like
  // base::test::ScopedTaskEnvironment
  content::TestBrowserThreadBundle thread_bundle_;

  BookmarkModel* model_;  // Not owns
  std::unique_ptr<Profile> profile_;
  base::ScopedTempDir temp_dir_;
};

TEST_F(SyncerHelperTest, AddBraveMetaInfoCreateOrUpdate) {
  std::string order;
  std::string sync_timestamp;
  const auto* folder1 = model()->AddFolder(model()->bookmark_bar_node(), 0,
                                           base::ASCIIToUTF16("Folder1"));
  AddBraveMetaInfo(folder1, model(), false);
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

  const auto *node_a = model()->AddURL(folder1, 0,
                                       base::ASCIIToUTF16("A.com - title"),
                                       GURL("https://a.com/"));
  order.clear();
  sync_timestamp.clear();
  AddBraveMetaInfo(node_a, model(), false);
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
  AddBraveMetaInfo(node_a, model(), false);
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
  AddBraveMetaInfo(folder1, model(), false);
  const auto *node_a = model()->AddURL(folder1, 0,
                                       base::ASCIIToUTF16("A.com - title"),
                                       GURL("https://a.com/"));
  AddBraveMetaInfo(node_a, model(), false);
  model()->Move(node_a, model()->bookmark_bar_node(), 1);
  AddBraveMetaInfo(node_a, model(), true);

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
  const auto *node_a = model()->AddURL(model()->bookmark_bar_node(), 0,
                                       base::ASCIIToUTF16("A.com - title"),
                                       GURL("https://a.com/"));
  AddBraveMetaInfo(node_a, model(), false);
  const auto *node_b = model()->AddURL(model()->bookmark_bar_node(), 1,
                                       base::ASCIIToUTF16("B.com - title"),
                                       GURL("https://b.com/"));
  AddBraveMetaInfo(node_b, model(), false);
  const auto *node_c = model()->AddURL(model()->bookmark_bar_node(), 2,
                                       base::ASCIIToUTF16("C.com - title"),
                                       GURL("https://c.com/"));
  AddBraveMetaInfo(node_c, model(), false);
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
  AddBraveMetaInfo(node_a, model(), false);
  AddBraveMetaInfo(node_b, model(), false);
  AddBraveMetaInfo(node_c, model(), false);

  order_a.clear();
  order_b.clear();
  order_c.clear();
 
  node_a->GetMetaInfo("order", &order_a);
  EXPECT_EQ(order_a, "1.0.1.2");
  node_b->GetMetaInfo("order", &order_b);
  EXPECT_EQ(order_b, "1.0.1.3");
  node_c->GetMetaInfo("order", &order_c);
  EXPECT_EQ(order_c, "1.0.1.1");
}

TEST_F(SyncerHelperTest, AddBraveMetaInfoNodeMovedReordered) {
  const auto *node_a = model()->AddURL(model()->bookmark_bar_node(), 0,
                                       base::ASCIIToUTF16("A.com - title"),
                                       GURL("https://a.com/"));
  AddBraveMetaInfo(node_a, model(), false);
  const auto* folder1 = model()->AddFolder(model()->bookmark_bar_node(), 1,
                                           base::ASCIIToUTF16("Folder1"));
  AddBraveMetaInfo(folder1, model(), false);
  const auto *node_b = model()->AddURL(folder1, 0,
                                       base::ASCIIToUTF16("B.com - title"),
                                       GURL("https://b.com/"));
  AddBraveMetaInfo(node_b, model(), false);
  const auto *node_c = model()->AddURL(folder1, 1,
                                       base::ASCIIToUTF16("C.com - title"),
                                       GURL("https://c.com/"));
  AddBraveMetaInfo(node_c, model(), false);

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
  AddBraveMetaInfo(node_a, model(), true);
  AddBraveMetaInfo(folder1, model(), false);
  AddBraveMetaInfo(node_b, model(), false);
  AddBraveMetaInfo(node_c, model(), false);

  order_a.clear();
  order_b.clear();
  order_c.clear();
  order_folder1.clear();

  node_a->GetMetaInfo("order", &order_a);
  EXPECT_EQ(order_a, "1.0.1.1.1");
  node_b->GetMetaInfo("order", &order_b);
  EXPECT_EQ(order_b, "1.0.1.1.2");
  node_c->GetMetaInfo("order", &order_c);
  EXPECT_EQ(order_c, "1.0.1.1.3");
  folder1->GetMetaInfo("order", &order_folder1);
  EXPECT_EQ(order_folder1, "1.0.1.1");
}

TEST_F(SyncerHelperTest, GetIndexInPermanentNodes) {
  BookmarkNode node(GURL("https://brave.com"));
  node.SetMetaInfo("order", "1.0.1.1");
  EXPECT_EQ(GetIndex(model()->bookmark_bar_node(), &node), 0u);

  node.SetMetaInfo("order", "1.0.2.1");
  EXPECT_EQ(GetIndex(model()->other_node(), &node), 0u);

  const auto* node_a = model()->AddURL(model()->bookmark_bar_node(), 0,
                                       base::ASCIIToUTF16("a.com"),
                                       GURL("https://a.com/"));
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
    const auto* node_a = model()->AddURL(model()->bookmark_bar_node(), i,
                                         base::ASCIIToUTF16("a.com"),
                                         GURL("https://a.com/"));
    std::string order = "1.1.1." + base::NumberToString(i);
    model()->SetNodeMetaInfo(node_a, "order", order);
  }
  // inserted as 10th child
  BookmarkNode node(GURL("https://brave.com"));
  node.SetMetaInfo("order", "1.0.1.10");
  EXPECT_EQ(GetIndex(model()->bookmark_bar_node(), &node), 9u);
  node.SetMetaInfo("order", "1.1.1.10");
  EXPECT_EQ(GetIndex(model()->bookmark_bar_node(), &node), 10u);
}

TEST_F(SyncerHelperTest, GetIndexInFolder) {
  const auto* folder1 = model()->AddFolder(model()->bookmark_bar_node(), 0,
                                           base::ASCIIToUTF16("Folder1"));
  model()->SetNodeMetaInfo(folder1, "order", "1.0.1.1");
  BookmarkNode node(GURL("https://brave.com"));
  node.SetMetaInfo("order", "1.0.1.1.1");
  EXPECT_EQ(GetIndex(folder1, &node), 0u);

  // appended at the end
  const auto* node_a = model()->AddURL(folder1, 0,
                                       base::ASCIIToUTF16("a.com"),
                                       GURL("https://a.com/"));
  model()->SetNodeMetaInfo(node_a, "order", "1.0.1.1.1");
  node.SetMetaInfo("order", "1.0.1.1.2");
  EXPECT_EQ(GetIndex(folder1, &node), 1u);
}

}   // namespace brave_sync
