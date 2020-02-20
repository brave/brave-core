/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../components/bookmarks/browser/bookmark_model_unittest.cc"
namespace bookmarks {

TEST_F(BookmarkModelTest, BraveMigrateOtherNodeFolder) {
  // -- Bookmarks
  // |-- A
  // |-- Other Bookmarks
  //     |-- B
  //     |   |--B1.com
  //     |-- C.com
  const bookmarks::BookmarkNode* other_node_folder =
      model_->AddFolder(model_->bookmark_bar_node(),
                        model_->bookmark_bar_node()->children().size(),
                        model_->other_node()->GetTitledUrlNodeTitle());
  model_->AddFolder(model_->bookmark_bar_node(), 0, ASCIIToUTF16("A"));
  const BookmarkNode* folder =
      model_->AddFolder(other_node_folder, 0, ASCIIToUTF16("B"));
  model_->AddURL(folder, 0, ASCIIToUTF16("B1"), GURL("https://B1.com"));

  model_->AddURL(other_node_folder, 1, ASCIIToUTF16("C.com"),
                 GURL("https://C.com"));
  // After migration, it should be
  // -- Bookmarks
  // |-- A
  // -- Other Bookmarks
  // |-- B
  // |   |--B1.com
  // |-- C.com
  BraveMigrateOtherNodeFolder(model_.get());
  ASSERT_EQ(model_->other_node()->children().size(), 2u);
  ASSERT_EQ(model_->bookmark_bar_node()->children().size(), 1u);
  EXPECT_EQ(model_->bookmark_bar_node()->children()[0]->GetTitle(),
            ASCIIToUTF16("A"));
  EXPECT_EQ(model_->other_node()->children()[0]->GetTitle(), ASCIIToUTF16("B"));
  EXPECT_EQ(model_->other_node()->children()[0]->children()[0]->GetTitle(),
            ASCIIToUTF16("B1"));
  EXPECT_EQ(model_->other_node()->children()[1]->GetTitle(),
            ASCIIToUTF16("C.com"));

  // Empty folder
  model_->AddFolder(model_->bookmark_bar_node(),
                    model_->bookmark_bar_node()->children().size(),
                    model_->other_node()->GetTitledUrlNodeTitle());
  BraveMigrateOtherNodeFolder(model_.get());
  ASSERT_EQ(model_->bookmark_bar_node()->children().size(), 1u);
  ASSERT_EQ(model_->other_node()->children().size(), 2u);
}

TEST_F(BookmarkModelTest, BraveMigrateOtherNodeFolderNotExist) {
  ASSERT_EQ(model_->bookmark_bar_node()->children().size(), 0u);
  BraveMigrateOtherNodeFolder(model_.get());
  ASSERT_EQ(model_->other_node()->children().size(), 0u);

  const BookmarkNode* folder =
    model_->AddFolder(model_->bookmark_bar_node(), 0, ASCIIToUTF16("Other B"));
  model_->AddURL(folder, 0, ASCIIToUTF16("B1"), GURL("https://B1.com"));
  BraveMigrateOtherNodeFolder(model_.get());
  ASSERT_EQ(model_->bookmark_bar_node()->children().size(), 1u);
  ASSERT_EQ(model_->other_node()->children().size(), 0u);

  model_->AddURL(model_->bookmark_bar_node(), 1,
                 model_->other_node()->GetTitledUrlNodeTitle(),
                 GURL("https://other.bookmarks"));
  BraveMigrateOtherNodeFolder(model_.get());
  ASSERT_EQ(model_->bookmark_bar_node()->children().size(), 2u);
  ASSERT_EQ(model_->other_node()->children().size(), 0u);
}

}  // namespace bookmarks
