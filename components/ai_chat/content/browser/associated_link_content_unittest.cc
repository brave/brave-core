// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/associated_link_content.h"

#include <memory>
#include <string>
#include <utility>

#include "base/run_loop.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/ai_chat/content/browser/page_content_fetcher.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "base/notreached.h"
#include "base/types/pass_key.h"
#include "components/tab_groups/tab_group_id.h"
#include "components/tabs/public/split_tab_id.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_interface.h"
#include "ui/base/unowned_user_data/unowned_user_data_host.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using base::test::RunOnceCallback;
using testing::_;
using testing::Return;
using testing::StrictMock;

namespace ai_chat {

namespace {

class MockPageContentFetcher : public PageContentFetcher {
 public:
  explicit MockPageContentFetcher(content::WebContents* web_contents)
      : PageContentFetcher(web_contents) {}

  MOCK_METHOD(void,
              FetchPageContent,
              (std::string_view, FetchPageContentCallback));
};

// Minimal test TabInterface that only implements GetContents()
class TestTabInterface : public tabs::TabInterface {
 public:
  explicit TestTabInterface(content::WebContents* web_contents)
      : web_contents_(web_contents) {}

  // Only implement the method that AssociatedLinkContent actually uses
  content::WebContents* GetContents() const override {
    return web_contents_;
  }

  // All other methods are stubs that should not be called in our tests
  base::WeakPtr<TabInterface> GetWeakPtr() override {
    NOTREACHED();
  }
  void Close() override { NOTREACHED(); }
  ui::UnownedUserDataHost& GetUnownedUserDataHost() override { NOTREACHED(); }
  const ui::UnownedUserDataHost& GetUnownedUserDataHost() const override { NOTREACHED(); }
  base::CallbackListSubscription RegisterWillDiscardContents(
      TabInterface::WillDiscardContentsCallback callback) override { NOTREACHED(); }
  bool IsActivated() const override { NOTREACHED(); }
  base::CallbackListSubscription RegisterDidActivate(
      TabInterface::DidActivateCallback callback) override { NOTREACHED(); }
  base::CallbackListSubscription RegisterWillDeactivate(
      TabInterface::WillDeactivateCallback callback) override { NOTREACHED(); }
  bool IsVisible() const override { NOTREACHED(); }
  bool IsSelected() const override { NOTREACHED(); }
  base::CallbackListSubscription RegisterDidBecomeVisible(
      DidBecomeVisibleCallback callback) override { NOTREACHED(); }
  base::CallbackListSubscription RegisterWillBecomeHidden(
      WillBecomeHiddenCallback callback) override { NOTREACHED(); }
  base::CallbackListSubscription RegisterWillDetach(
      WillDetach callback) override { NOTREACHED(); }
  base::CallbackListSubscription RegisterDidInsert(
      DidInsertCallback callback) override { NOTREACHED(); }
  base::CallbackListSubscription RegisterPinnedStateChanged(
      PinnedStateChangedCallback callback) override { NOTREACHED(); }
  base::CallbackListSubscription RegisterGroupChanged(
      GroupChangedCallback callback) override { NOTREACHED(); }
  bool CanShowModalUI() const override { NOTREACHED(); }
  std::unique_ptr<tabs::ScopedTabModalUI> ShowModalUI() override { NOTREACHED(); }
  base::CallbackListSubscription RegisterModalUIChanged(
      TabInterfaceCallback callback) override { NOTREACHED(); }
  bool IsInNormalWindow() const override { NOTREACHED(); }
  BrowserWindowInterface* GetBrowserWindowInterface() override { NOTREACHED(); }
  const BrowserWindowInterface* GetBrowserWindowInterface() const override { NOTREACHED(); }
  tabs::TabFeatures* GetTabFeatures() override { NOTREACHED(); }
  const tabs::TabFeatures* GetTabFeatures() const override { NOTREACHED(); }
  bool IsPinned() const override { NOTREACHED(); }
  bool IsSplit() const override { NOTREACHED(); }
  std::optional<tab_groups::TabGroupId> GetGroup() const override { NOTREACHED(); }
  std::optional<split_tabs::SplitTabId> GetSplit() const override { NOTREACHED(); }
  tabs::TabCollection* GetParentCollection(
      base::PassKey<tabs::TabCollection>) const override { NOTREACHED(); }
  const tabs::TabCollection* GetParentCollection() const override { NOTREACHED(); }
  void OnReparented(tabs::TabCollection* parent,
                    base::PassKey<tabs::TabCollection>) override { NOTREACHED(); }
  void OnAncestorChanged(base::PassKey<tabs::TabCollection>) override { NOTREACHED(); }

 private:
  raw_ptr<content::WebContents> web_contents_;
};

}  // namespace

// Test wrapper that exposes private methods for testing
class TestableAssociatedLinkContent : public AssociatedLinkContent {
 public:
  TestableAssociatedLinkContent(GURL url,
                                std::u16string title,
                                tabs::TabInterface* tab_interface)
      : AssociatedLinkContent(std::move(url),
                              std::move(title),
                              tab_interface) {
    auto fetcher =
        std::make_unique<MockPageContentFetcher>(tab_interface->GetContents());
    mock_fetcher_ = fetcher.get();
    content_fetcher_ = std::move(fetcher);
  }

  ~TestableAssociatedLinkContent() override { mock_fetcher_ = nullptr; }

  using AssociatedContentDelegate::set_cached_page_content;
  using AssociatedLinkContent::DidFinishNavigation;
  using AssociatedLinkContent::DocumentOnLoadCompletedInPrimaryMainFrame;
  using AssociatedLinkContent::OnContentExtractionComplete;
  using AssociatedLinkContent::OnTimeout;
  using AssociatedLinkContent::tab_interface_;

  base::OneShotEvent* content_loaded_event() {
    return content_loaded_event_.get();
  }

  void set_page_content_result(std::string content, bool is_video) {
    EXPECT_CALL(*mock_fetcher_, FetchPageContent(_, _))
        .WillOnce(RunOnceCallback<1>(content, is_video, ""));
  }

 private:
  raw_ptr<MockPageContentFetcher> mock_fetcher_;
};

class AssociatedLinkContentTest : public content::RenderViewHostTestHarness {
 public:
  AssociatedLinkContentTest()
      : content::RenderViewHostTestHarness(
            base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~AssociatedLinkContentTest() override = default;

  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();
    test_tab_interface_ = std::make_unique<TestTabInterface>(web_contents());
  }

  void TearDown() override {
    // Clean up TestTabInterface before the base class cleans up WebContents
    test_tab_interface_.reset();
    content::RenderViewHostTestHarness::TearDown();
  }

  std::unique_ptr<TestableAssociatedLinkContent> CreateAssociatedLinkContent(
      GURL url,
      std::u16string title) {
    return std::make_unique<TestableAssociatedLinkContent>(
        url, title, test_tab_interface_.get());
  }

 protected:
  GURL test_url() { return GURL("https://example.com/test-page"); }
  std::u16string test_title() { return u"Test Page Title"; }
  std::string test_content() { return "This is test page content"; }

 private:
  std::unique_ptr<TestTabInterface> test_tab_interface_;
};

TEST_F(AssociatedLinkContentTest, ConstructorDoesNotTriggerLoad) {
  auto associated_content =
      CreateAssociatedLinkContent(test_url(), test_title());
  EXPECT_FALSE(associated_content->content_loaded_event());
  EXPECT_EQ(associated_content->url(), test_url());
  EXPECT_EQ(associated_content->title(), test_title());
  EXPECT_FALSE(associated_content->uuid().empty());
  EXPECT_TRUE(associated_content->cached_page_content().content.empty());
  EXPECT_FALSE(associated_content->cached_page_content().is_video);
}

TEST_F(AssociatedLinkContentTest,
       GetContentResolvesWhenDocumentOnLoadCompletedInPrimaryMainFrame) {
  auto associated_content =
      CreateAssociatedLinkContent(test_url(), test_title());
  associated_content->set_page_content_result(test_content(), false);

  base::test::TestFuture<PageContent> future;
  associated_content->GetContent(future.GetCallback());
  EXPECT_TRUE(associated_content->content_loaded_event());

  associated_content->DocumentOnLoadCompletedInPrimaryMainFrame();

  const auto [content, is_video] = future.Take();
  EXPECT_EQ(content, test_content());
  EXPECT_FALSE(is_video);
}

TEST_F(AssociatedLinkContentTest, GetContentWhenCachePopulated) {
  auto testable_content = CreateAssociatedLinkContent(test_url(), test_title());

  PageContent cached_content(test_content(), false);
  testable_content->set_cached_page_content(cached_content);

  base::test::TestFuture<PageContent> future;
  testable_content->GetContent(future.GetCallback());

  const PageContent& result = future.Take();
  EXPECT_EQ(result.content, test_content());
  EXPECT_FALSE(result.is_video);
}

TEST_F(AssociatedLinkContentTest, MultipleGetContentCallsWithEmptyCache) {
  auto associated_content =
      CreateAssociatedLinkContent(test_url(), test_title());
  associated_content->set_page_content_result(test_content(), false);

  base::test::TestFuture<PageContent> future1;
  base::test::TestFuture<PageContent> future2;

  associated_content->GetContent(future1.GetCallback());
  associated_content->GetContent(future2.GetCallback());

  EXPECT_FALSE(future1.IsReady());
  EXPECT_FALSE(future2.IsReady());

  associated_content->DocumentOnLoadCompletedInPrimaryMainFrame();

  const auto& [content1, is_video1] = future1.Take();
  EXPECT_EQ(content1, test_content());
  EXPECT_FALSE(is_video1);

  const auto& [content2, is_video2] = future2.Take();
  EXPECT_EQ(content2, test_content());
  EXPECT_FALSE(is_video2);
}

TEST_F(AssociatedLinkContentTest, DidFinishNavigationWithError) {
  auto associated_content =
      CreateAssociatedLinkContent(test_url(), test_title());

  base::test::TestFuture<PageContent> future;
  associated_content->GetContent(future.GetCallback());

  content::MockNavigationHandle mock_handle(test_url(), main_rfh());
  mock_handle.set_has_committed(true);
  mock_handle.set_is_error_page(true);
  mock_handle.set_net_error_code(net::ERR_CONNECTION_FAILED);

  associated_content->DidFinishNavigation(&mock_handle);

  const PageContent& result = future.Take();
  EXPECT_TRUE(result.content.empty());
  EXPECT_FALSE(result.is_video);
}

TEST_F(AssociatedLinkContentTest, DidFinishNavigationNonPrimaryFrame) {
  auto associated_content =
      CreateAssociatedLinkContent(test_url(), test_title());

  base::test::TestFuture<PageContent> future;
  associated_content->GetContent(future.GetCallback());

  content::MockNavigationHandle mock_handle(test_url(), main_rfh());
  mock_handle.set_has_committed(true);
  mock_handle.set_is_error_page(true);
  mock_handle.set_is_in_primary_main_frame(false);

  associated_content->DidFinishNavigation(&mock_handle);

  task_environment()->RunUntilIdle();
  EXPECT_FALSE(future.IsReady());
}

TEST_F(AssociatedLinkContentTest, DidFinishNavigationNotCommitted) {
  auto associated_content =
      CreateAssociatedLinkContent(test_url(), test_title());

  base::test::TestFuture<PageContent> future;
  associated_content->GetContent(future.GetCallback());

  content::MockNavigationHandle mock_handle(test_url(), main_rfh());
  mock_handle.set_has_committed(false);
  mock_handle.set_is_error_page(true);
  mock_handle.set_is_in_primary_main_frame(true);

  associated_content->DidFinishNavigation(&mock_handle);

  task_environment()->RunUntilIdle();
  EXPECT_FALSE(future.IsReady());
}

TEST_F(AssociatedLinkContentTest, OnContentExtractionCompleteSetsCache) {
  auto associated_content =
      CreateAssociatedLinkContent(test_url(), test_title());

  base::test::TestFuture<PageContent> future;
  associated_content->GetContent(future.GetCallback());

  EXPECT_TRUE(associated_content->cached_page_content().content.empty());

  associated_content->OnContentExtractionComplete(test_content(), true,
                                                  "invalidation_token");

  const auto& [content, is_video] = future.Take();
  EXPECT_EQ(content, test_content());
  EXPECT_TRUE(is_video);

  const PageContent& cached = associated_content->cached_page_content();
  EXPECT_EQ(cached.content, test_content());
  EXPECT_TRUE(cached.is_video);
}

TEST_F(AssociatedLinkContentTest, TimeoutClearsContentAndCompletesCallback) {
  auto associated_content =
      CreateAssociatedLinkContent(test_url(), test_title());

  base::test::TestFuture<PageContent> future;
  associated_content->GetContent(future.GetCallback());

  // Fast forward past the 30 second timeout
  task_environment()->FastForwardBy(base::Seconds(31));

  // Verify the callback was invoked with empty content
  const auto& [content, is_video] = future.Take();
  EXPECT_TRUE(content.empty());
  EXPECT_FALSE(is_video);
}

}  // namespace ai_chat
