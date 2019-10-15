/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/bookmarks/test/test_bookmark_client.h"

namespace bookmarks {
class BraveTestBookmarkClient : public TestBookmarkClient {
 public:
  BraveTestBookmarkClient() : TestBookmarkClient() {}
  ~BraveTestBookmarkClient() override {}

  static std::unique_ptr<BookmarkModel> CreateModel() {
  return CreateModelWithClient(base::WrapUnique(new BraveTestBookmarkClient));
  }

 private:
  // BookmarkClient:
  bool IsPermanentNodeVisible(const BookmarkPermanentNode* node) override {
    if (node->type() == bookmarks::BookmarkNode::OTHER_NODE)
      return false;
    return TestBookmarkClient::IsPermanentNodeVisible(node);
  }
  DISALLOW_COPY_AND_ASSIGN(BraveTestBookmarkClient);
};
}  // namespace bookmarks

#define TestBookmarkClient BraveTestBookmarkClient
#define NodeVisibility DISABLED_NodeVisibility
#define MostRecentlyModifiedFolders DISABLED_MostRecentlyModifiedFolders
#include "../../../../../components/bookmarks/browser/bookmark_model_unittest.cc"
#undef TestBookmarkClient
#undef MostRecentlyModifiedFolders
#undef NodeVisibility
namespace bookmarks {

TEST_F(BookmarkModelTest, BraveNodeVisibility) {
  // Mobile node invisible by default
  EXPECT_TRUE(model_->bookmark_bar_node()->IsVisible());
  // Other node invisible by default
  EXPECT_FALSE(model_->other_node()->IsVisible());
  EXPECT_FALSE(model_->mobile_node()->IsVisible());

  // Visibility of permanent node can only be changed if they are not
  // forced to be visible by the client.
  model_->SetPermanentNodeVisible(BookmarkNode::BOOKMARK_BAR, false);
  EXPECT_TRUE(model_->bookmark_bar_node()->IsVisible());
  model_->SetPermanentNodeVisible(BookmarkNode::OTHER_NODE, false);
  EXPECT_FALSE(model_->other_node()->IsVisible());
  model_->SetPermanentNodeVisible(BookmarkNode::MOBILE, true);
  EXPECT_TRUE(model_->mobile_node()->IsVisible());
  model_->SetPermanentNodeVisible(BookmarkNode::MOBILE, false);
  EXPECT_FALSE(model_->mobile_node()->IsVisible());

  // Arbitrary node should be visible
  TestNode bbn;
  PopulateNodeFromString("B", &bbn);
  const BookmarkNode* parent = model_->mobile_node();
  PopulateBookmarkNode(&bbn, model_.get(), parent);
  EXPECT_TRUE(parent->children().front()->IsVisible());

  // Mobile folder should be visible now that it has a child.
  EXPECT_TRUE(model_->mobile_node()->IsVisible());
}

// Make sure recently modified stays in sync when adding a URL.
TEST_F(BookmarkModelTest, BraveMostRecentlyModifiedFolders) {
  // Add a folder.
  const BookmarkNode* folder =
      model_->AddFolder(model_->bookmark_bar_node(), 0, ASCIIToUTF16("foo"));
  // Add a URL to it.
  model_->AddURL(folder, 0, ASCIIToUTF16("blah"), GURL("http://foo.com"));

  // Make sure folder is in the most recently modified.
  std::vector<const BookmarkNode*> most_recent_folders =
      GetMostRecentlyModifiedUserFolders(model_.get(), 1);
  ASSERT_EQ(1U, most_recent_folders.size());
  ASSERT_EQ(folder, most_recent_folders[0]);

  // Nuke the folder and do another fetch, making sure folder isn't in the
  // returned list.
  model_->Remove(folder->parent()->children().front().get());
  most_recent_folders = GetMostRecentlyModifiedUserFolders(model_.get(), 1);
  ASSERT_EQ(1U, most_recent_folders.size());
  ASSERT_TRUE(most_recent_folders[0] != folder);
}

TEST_F(BookmarkModelTest, MigrateOtherNode) {
  // -- Bookmarks
  // |-- A
  // -- Other Bookmarks
  // |-- B
  // |   |--B1.com
  // |-- C.com
  model_->AddFolder(model_->bookmark_bar_node(), 0, ASCIIToUTF16("A"));
  const BookmarkNode* folder =
      model_->AddFolder(model_->other_node(), 0, ASCIIToUTF16("B"));
  model_->AddURL(folder, 0, ASCIIToUTF16("B1"), GURL("https://B1.com"));

  model_->AddURL(model_->other_node(), 1, ASCIIToUTF16("C"),
                 GURL("https://B.com"));
  // After migration, it should be
  // -- Bookmarks
  // |-- A
  // |-- Other Bookmarks
  //     |-- B
  //     |   |--B1.com
  //     |-- C.com
  model_->MigrateOtherNode();
  ASSERT_EQ(model_->other_node()->children().size(), 0u);
  ASSERT_EQ(model_->bookmark_bar_node()->children().size(), 2u);
  EXPECT_EQ(model_->bookmark_bar_node()->children()[0]->GetTitle(),
            ASCIIToUTF16("A"));
  EXPECT_EQ(model_->bookmark_bar_node()->children()[1]->GetTitle(),
            model_->other_node()->GetTitle());
  const BookmarkNode* new_other_node =
      model_->bookmark_bar_node()->children()[1].get();
  EXPECT_EQ(new_other_node->children()[0]->GetTitle(), ASCIIToUTF16("B"));
  EXPECT_EQ(new_other_node->children()[0]->children()[0]->GetTitle(),
            ASCIIToUTF16("B1"));
  EXPECT_EQ(new_other_node->children()[1]->GetTitle(), ASCIIToUTF16("C"));
}

}  // namespace bookmarks
