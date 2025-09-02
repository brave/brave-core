// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/bookmarks_service.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/ai_chat/core/common/mojom/bookmarks.mojom.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/test/test_bookmark_client.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

// Mock BookmarksListener for testing
class MockBookmarksListener : public mojom::BookmarksListener {
 public:
  MockBookmarksListener() = default;
  ~MockBookmarksListener() override = default;

  // mojom::BookmarksListener:
  void Changed(mojom::BookmarksChangePtr change) override {
    last_change_ = std::move(change);
    if (run_loop_) {
      run_loop_->Quit();
    }
  }

  void WaitForChange() {
    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop_->Run();
  }

  mojom::BookmarksChangePtr TakeLastChange() { return std::move(last_change_); }

  bool HasReceivedChange() const { return !!last_change_; }

 private:
  std::unique_ptr<base::RunLoop> run_loop_;
  mojom::BookmarksChangePtr last_change_;
};

}  // namespace

class BookmarksServiceTest : public testing::Test {
 public:
  BookmarksServiceTest() = default;
  ~BookmarksServiceTest() override = default;

  void SetUp() override {
    // Create BookmarkModel with test client - model loads synchronously in
    // tests
    bookmark_model_ = bookmarks::TestBookmarkClient::CreateModel();

    // Create BookmarksService with mojo receiver
    mojo::PendingReceiver<mojom::BookmarksService> receiver =
        bookmarks_service_remote_.BindNewPipeAndPassReceiver();
    bookmarks_service_ = std::make_unique<BookmarksService>(
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

  const bookmarks::BookmarkNode* GetBookmarkByTitle(const std::string& title) {
    // Search through all nodes
    return FindBookmarkByTitle(bookmark_model_->root_node(), title);
  }

  const bookmarks::BookmarkNode* FindBookmarkByTitle(
      const bookmarks::BookmarkNode* parent,
      const std::string& title) {
    if (!parent->is_folder() &&
        base::UTF16ToUTF8(parent->GetTitle()) == title) {
      return parent;
    }

    for (const auto& child : parent->children()) {
      if (const auto* found = FindBookmarkByTitle(child.get(), title)) {
        return found;
      }
    }
    return nullptr;
  }

  std::unique_ptr<MockBookmarksListener> CreateMockListener() {
    auto listener = std::make_unique<MockBookmarksListener>();
    mojo::PendingRemote<mojom::BookmarksListener> remote;
    auto receiver = std::make_unique<mojo::Receiver<mojom::BookmarksListener>>(
        listener.get(), remote.InitWithNewPipeAndPassReceiver());

    bookmarks_service_remote_->AddListener(std::move(remote));
    base::RunLoop().RunUntilIdle();  // Process the AddListener call

    // Keep receiver alive
    listener_receivers_.push_back(std::move(receiver));

    return listener;
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<bookmarks::BookmarkModel> bookmark_model_;
  std::unique_ptr<BookmarksService> bookmarks_service_;
  mojo::Remote<mojom::BookmarksService> bookmarks_service_remote_;
  std::vector<std::unique_ptr<mojo::Receiver<mojom::BookmarksListener>>>
      listener_receivers_;
};

TEST_F(BookmarksServiceTest, InitialBookmarksLoad) {
  // Add some bookmarks before creating listener
  AddTestBookmark("Test Bookmark 1", GURL("https://example.com"));
  AddTestBookmark("Test Bookmark 2", GURL("https://brave.com"));

  // Create listener - should receive initial bookmarks immediately
  auto listener = CreateMockListener();

  // Listener should have received the initial bookmarks
  ASSERT_TRUE(listener->HasReceivedChange());
  auto change = listener->TakeLastChange();

  EXPECT_EQ(2u, change->addedOrUpdated.size());
  EXPECT_TRUE(change->removed.empty());

  // Verify bookmark data
  EXPECT_NE(change->addedOrUpdated.find("https://example.com/"),
            change->addedOrUpdated.end());
  EXPECT_NE(change->addedOrUpdated.find("https://brave.com/"),
            change->addedOrUpdated.end());

  auto& bookmark1 = change->addedOrUpdated["https://example.com/"];
  EXPECT_EQ("Test Bookmark 1", bookmark1->title);
  EXPECT_EQ(GURL("https://example.com"), bookmark1->url);

  auto& bookmark2 = change->addedOrUpdated["https://brave.com/"];
  EXPECT_EQ("Test Bookmark 2", bookmark2->title);
  EXPECT_EQ(GURL("https://brave.com"), bookmark2->url);
}

TEST_F(BookmarksServiceTest, BookmarkAddedNotification) {
  auto listener = CreateMockListener();

  // Add a new bookmark
  AddTestBookmark("New Bookmark", GURL("https://new.com"));

  listener->WaitForChange();
  auto change = listener->TakeLastChange();

  EXPECT_EQ(1u, change->addedOrUpdated.size());
  EXPECT_TRUE(change->removed.empty());

  auto& bookmark = change->addedOrUpdated["https://new.com/"];
  EXPECT_EQ("New Bookmark", bookmark->title);
  EXPECT_EQ(GURL("https://new.com"), bookmark->url);
}

TEST_F(BookmarksServiceTest, BookmarkRemovedNotification) {
  // Add bookmark first
  auto* bookmark = AddTestBookmark("To Remove", GURL("https://remove.com"));
  auto listener = CreateMockListener();

  // Remove the bookmark
  RemoveTestBookmark(bookmark);

  listener->WaitForChange();
  auto change = listener->TakeLastChange();

  EXPECT_TRUE(change->addedOrUpdated.empty());
  EXPECT_EQ(1u, change->removed.size());
  EXPECT_EQ("https://remove.com/", change->removed[0]);
}

TEST_F(BookmarksServiceTest, BookmarkChangedNotification) {
  // Add bookmark first
  auto* bookmark =
      AddTestBookmark("Original Title", GURL("https://change.com"));

  auto listener = CreateMockListener();

  // Change the bookmark title
  bookmark_model_->SetTitle(bookmark, u"Changed Title",
                            bookmarks::metrics::BookmarkEditSource::kOther);

  listener->WaitForChange();
  auto change = listener->TakeLastChange();

  EXPECT_EQ(1u, change->addedOrUpdated.size());
  EXPECT_TRUE(change->removed.empty());

  auto& changed_bookmark = change->addedOrUpdated["https://change.com/"];
  EXPECT_EQ("Changed Title", changed_bookmark->title);
  EXPECT_EQ(GURL("https://change.com"), changed_bookmark->url);
}

TEST_F(BookmarksServiceTest, FolderBookmarksIgnored) {
  auto listener = CreateMockListener();

  // Clear initial load
  listener->TakeLastChange();

  // Add a folder - should not trigger notification
  bookmark_model_->AddFolder(bookmark_model_->bookmark_bar_node(), 0,
                             u"Test Folder");

  // Wait a bit to ensure no notification is sent
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(listener->HasReceivedChange());
}

TEST_F(BookmarksServiceTest, AllUserNodesRemovedNotification) {
  // Add some bookmarks
  AddTestBookmark("Bookmark 1", GURL("https://one.com"));
  AddTestBookmark("Bookmark 2", GURL("https://two.com"));

  auto listener = CreateMockListener();

  // Remove all user nodes
  bookmark_model_->RemoveAllUserBookmarks(FROM_HERE);

  listener->WaitForChange();
  auto change = listener->TakeLastChange();

  // Should receive fresh bookmark list (empty) with removed URLs
  EXPECT_TRUE(change->addedOrUpdated.empty());
  EXPECT_EQ(2u, change->removed.size());

  // Check that removed URLs are included
  std::vector<std::string> removed_urls = change->removed;
  EXPECT_TRUE(base::Contains(removed_urls, "https://one.com/"));
  EXPECT_TRUE(base::Contains(removed_urls, "https://two.com/"));
}

TEST_F(BookmarksServiceTest, MultipleListeners) {
  auto listener1 = CreateMockListener();
  auto listener2 = CreateMockListener();

  // Both should receive initial bookmarks
  EXPECT_TRUE(listener1->HasReceivedChange());
  EXPECT_TRUE(listener2->HasReceivedChange());

  // Add a bookmark
  AddTestBookmark("Multi Test", GURL("https://multi.com"));

  // Both listeners should be notified
  listener1->WaitForChange();
  listener2->WaitForChange();

  auto change1 = listener1->TakeLastChange();
  auto change2 = listener2->TakeLastChange();

  ASSERT_EQ(1u, change1->addedOrUpdated.size());
  ASSERT_EQ(1u, change2->addedOrUpdated.size());

  EXPECT_EQ(change1->addedOrUpdated.begin()->second->title,
            change2->addedOrUpdated.begin()->second->title);
}

TEST_F(BookmarksServiceTest, EmptyBookmarkModel) {
  // Create listener with empty model
  auto listener = CreateMockListener();

  // Should receive empty bookmark set when attached to empty model
  auto change = listener->TakeLastChange();

  EXPECT_TRUE(change->addedOrUpdated.empty());
  EXPECT_TRUE(change->removed.empty());
}

}  // namespace ai_chat
