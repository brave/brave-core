// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/bookmarks_page_handler.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/ai_chat/core/common/mojom/bookmarks.mojom.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/test/test_bookmark_client.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

class BookmarksPageHandlerTest : public testing::Test {
 public:
  BookmarksPageHandlerTest() = default;
  ~BookmarksPageHandlerTest() override = default;

  void SetUp() override {
    // Create BookmarkModel with test client - model loads synchronously in
    // tests
    bookmark_model_ = bookmarks::TestBookmarkClient::CreateModel();

    // Create BookmarksPageHandler with mojo receiver
    mojo::PendingReceiver<mojom::BookmarksPageHandler> receiver =
        bookmarks_page_handler_remote_.BindNewPipeAndPassReceiver();
    bookmarks_page_handler_ = std::make_unique<BookmarksPageHandler>(
        bookmark_model_.get(), std::move(receiver));
  }

  const bookmarks::BookmarkNode* AddTestBookmark(const std::string& title,
                                                 const GURL& url) {
    return bookmark_model_->AddURL(bookmark_model_->bookmark_bar_node(), 0,
                                   base::UTF8ToUTF16(title), url);
  }

  void RemoveTestBookmark(const bookmarks::BookmarkNode* bookmark) {
    bookmark_model_->Remove(
        bookmark, bookmarks::metrics::BookmarkEditSource::kOther, FROM_HERE);
  }

  std::vector<mojom::BookmarkPtr> GetBookmarks() {
    base::test::TestFuture<std::vector<mojom::BookmarkPtr>> future;
    bookmarks_page_handler_remote_->GetBookmarks(future.GetCallback());
    return future.Take();
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<bookmarks::BookmarkModel> bookmark_model_;
  std::unique_ptr<BookmarksPageHandler> bookmarks_page_handler_;
  mojo::Remote<mojom::BookmarksPageHandler> bookmarks_page_handler_remote_;
};

TEST_F(BookmarksPageHandlerTest, EmptyBookmarkModel) {
  // Should return empty list when no bookmarks exist
  auto bookmarks = GetBookmarks();
  EXPECT_TRUE(bookmarks.empty());
}

TEST_F(BookmarksPageHandlerTest, GetMultipleBookmarks) {
  // Add multiple bookmarks
  AddTestBookmark("Bookmark 1", GURL("https://example1.com"));
  AddTestBookmark("Bookmark 2", GURL("https://example2.com"));
  AddTestBookmark("Bookmark 3", GURL("https://example3.com"));

  auto bookmarks = GetBookmarks();
  ASSERT_EQ(3u, bookmarks.size());

  // Find bookmarks by title (order may vary)
  bool found_bookmark1 = false, found_bookmark2 = false,
       found_bookmark3 = false;

  for (const auto& bookmark : bookmarks) {
    if (bookmark->title == "Bookmark 1" &&
        bookmark->url == GURL("https://example1.com")) {
      found_bookmark1 = true;
    } else if (bookmark->title == "Bookmark 2" &&
               bookmark->url == GURL("https://example2.com")) {
      found_bookmark2 = true;
    } else if (bookmark->title == "Bookmark 3" &&
               bookmark->url == GURL("https://example3.com")) {
      found_bookmark3 = true;
    }
  }

  EXPECT_TRUE(found_bookmark1);
  EXPECT_TRUE(found_bookmark2);
  EXPECT_TRUE(found_bookmark3);
}

TEST_F(BookmarksPageHandlerTest, BookmarksInNestedFolders) {
  // Create a nested folder structure
  auto* folder1 = bookmark_model_->AddFolder(
      bookmark_model_->bookmark_bar_node(), 0, u"Folder 1");
  auto* folder2 = bookmark_model_->AddFolder(folder1, 0, u"Folder 2");

  // Add bookmarks at different levels
  AddTestBookmark("Root Bookmark", GURL("https://root.com"));
  bookmark_model_->AddURL(folder1, 0, u"Folder1 Bookmark",
                          GURL("https://folder1.com"));
  bookmark_model_->AddURL(folder2, 0, u"Folder2 Bookmark",
                          GURL("https://folder2.com"));

  auto bookmarks = GetBookmarks();
  ASSERT_EQ(3u, bookmarks.size());

  // Verify all bookmarks are returned regardless of folder location
  std::vector<std::string> titles;
  for (const auto& bookmark : bookmarks) {
    titles.push_back(bookmark->title);
  }

  EXPECT_TRUE(std::find(titles.begin(), titles.end(), "Root Bookmark") !=
              titles.end());
  EXPECT_TRUE(std::find(titles.begin(), titles.end(), "Folder1 Bookmark") !=
              titles.end());
  EXPECT_TRUE(std::find(titles.begin(), titles.end(), "Folder2 Bookmark") !=
              titles.end());
}

TEST_F(BookmarksPageHandlerTest, FoldersNotIncluded) {
  // Add both folders and bookmarks
  bookmark_model_->AddFolder(bookmark_model_->bookmark_bar_node(), 0,
                             u"Test Folder");
  AddTestBookmark("Test Bookmark", GURL("https://example.com"));

  auto bookmarks = GetBookmarks();

  // Only the bookmark should be returned, not the folder
  ASSERT_EQ(1u, bookmarks.size());
  EXPECT_EQ("Test Bookmark", bookmarks[0]->title);
}

TEST_F(BookmarksPageHandlerTest, BookmarkRemoval) {
  // Add bookmarks
  auto* bookmark1 = AddTestBookmark("Bookmark 1", GURL("https://example1.com"));
  AddTestBookmark("Bookmark 2", GURL("https://example2.com"));

  // Verify both bookmarks exist
  auto bookmarks = GetBookmarks();
  ASSERT_EQ(2u, bookmarks.size());

  // Remove one bookmark
  RemoveTestBookmark(bookmark1);

  // Verify only one bookmark remains
  bookmarks = GetBookmarks();
  ASSERT_EQ(1u, bookmarks.size());
  EXPECT_EQ("Bookmark 2", bookmarks[0]->title);
}

TEST_F(BookmarksPageHandlerTest, BookmarkModification) {
  // Add a bookmark
  auto* bookmark =
      AddTestBookmark("Original Title", GURL("https://example.com"));

  // Verify initial state
  auto bookmarks = GetBookmarks();
  ASSERT_EQ(1u, bookmarks.size());
  EXPECT_EQ("Original Title", bookmarks[0]->title);

  // Modify the bookmark title
  bookmark_model_->SetTitle(bookmark, u"Modified Title",
                            bookmarks::metrics::BookmarkEditSource::kOther);

  // Verify the change is reflected
  bookmarks = GetBookmarks();
  ASSERT_EQ(1u, bookmarks.size());
  EXPECT_EQ("Modified Title", bookmarks[0]->title);
  EXPECT_EQ(GURL("https://example.com"), bookmarks[0]->url);
}

TEST_F(BookmarksPageHandlerTest, AllBookmarksRemoved) {
  // Add some bookmarks
  AddTestBookmark("Bookmark 1", GURL("https://example1.com"));
  AddTestBookmark("Bookmark 2", GURL("https://example2.com"));

  // Verify bookmarks exist
  auto bookmarks = GetBookmarks();
  ASSERT_EQ(2u, bookmarks.size());

  // Remove all bookmarks
  bookmark_model_->RemoveAllUserBookmarks(FROM_HERE);

  // Verify no bookmarks remain
  bookmarks = GetBookmarks();
  EXPECT_TRUE(bookmarks.empty());
}

}  // namespace ai_chat
