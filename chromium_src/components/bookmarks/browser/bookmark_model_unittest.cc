/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../components/bookmarks/browser/bookmark_model_unittest.cc"
namespace bookmarks {

TEST_F(BookmarkModelTest, BraveMigrateOtherNode) {
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
  BraveMigrateOtherNode(model_.get());
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
