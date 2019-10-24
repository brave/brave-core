/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../../../chrome/browser/extensions/api/bookmarks/bookmarks_api_unittest.cc"  // NOLINT

namespace extensions {

TEST_F(BookmarksApiUnittest, Create) {
  bookmarks::BookmarkModel* model =
    BookmarkModelFactory::GetForBrowserContext(profile());
  {
    auto create_function = base::MakeRefCounted<BookmarksCreateFunction>();
    const std::string other_node_id =
      base::NumberToString(model->other_node()->id());
    // Specify other_node() as parent
    const GURL url("https://brave.com");
    api_test_utils::RunFunction(
      create_function.get(),
      base::StringPrintf(
          R"([{"parentId": "%s",
               "title": "brave",
               "url": "%s"}])", other_node_id.c_str(), url.spec().c_str()),
      profile());
    auto* node = model->GetMostRecentlyAddedUserNodeForURL(url);
    EXPECT_EQ(node->url(), url);
    EXPECT_EQ(node->parent(), model->bookmark_bar_node());
  }
  {
    auto create_function = base::MakeRefCounted<BookmarksCreateFunction>();
    // default parent
    const GURL url("https://brave2.com");
    api_test_utils::RunFunction(
      create_function.get(),
      base::StringPrintf(
          R"([{"title": "brave2",
               "url": "%s"}])", url.spec().c_str()),
      profile());
    auto* node = model->GetMostRecentlyAddedUserNodeForURL(url);
    EXPECT_EQ(node->url(), url);
    EXPECT_EQ(node->parent(), model->bookmark_bar_node());
  }
}

TEST_F(BookmarksApiUnittest, Move) {
  bookmarks::BookmarkModel* model =
    BookmarkModelFactory::GetForBrowserContext(profile());
    const std::string other_node_id =
      base::NumberToString(model->other_node()->id());
  const bookmarks::BookmarkNode* node = model->AddURL(
        model->bookmark_bar_node(), 0, base::ASCIIToUTF16("brave"),
        GURL("https://brave.com"));
  auto move_function = base::MakeRefCounted<BookmarksMoveFunction>();
  api_test_utils::RunFunction(
    move_function.get(),
    base::StringPrintf(
        R"(["%s", {"parentId": "%s"}])",
        base::NumberToString(node->id()).c_str(), other_node_id.c_str()),
      profile());
  EXPECT_EQ(node->parent(), model->bookmark_bar_node());
}

}  // namespace extensions
