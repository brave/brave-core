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
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
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

class MockAssociatedLinkContentDelegate
    : public AssociatedLinkContent::Delegate {
 public:
  MockAssociatedLinkContentDelegate() = default;
  ~MockAssociatedLinkContentDelegate() override = default;
  MOCK_METHOD(void, AttachTabHelpers, (content::WebContents * web_contents));
};

}  // namespace

// Test wrapper that exposes private methods for testing
class TestableAssociatedLinkContent : public AssociatedLinkContent {
 public:
  TestableAssociatedLinkContent(GURL url,
                                std::u16string title,
                                content::BrowserContext* browser_context)
      : AssociatedLinkContent(
            std::move(url),
            std::move(title),
            browser_context,
            std::make_unique<MockAssociatedLinkContentDelegate>()) {
    auto fetcher =
        std::make_unique<MockPageContentFetcher>(web_contents_.get());
    mock_fetcher_ = fetcher.get();
    content_fetcher_ = std::move(fetcher);
  }

  ~TestableAssociatedLinkContent() override { mock_fetcher_ = nullptr; }

  using AssociatedContentDelegate::set_cached_page_content;
  using AssociatedLinkContent::DidFinishNavigation;
  using AssociatedLinkContent::DocumentOnLoadCompletedInPrimaryMainFrame;
  using AssociatedLinkContent::OnContentExtractionComplete;
  using AssociatedLinkContent::OnTimeout;
  using AssociatedLinkContent::web_contents_;

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

  std::unique_ptr<TestableAssociatedLinkContent> CreateAssociatedLinkContent(
      GURL url,
      std::u16string title) {
    return std::make_unique<TestableAssociatedLinkContent>(url, title,
                                                           browser_context());
  }

 protected:
  GURL test_url() { return GURL("https://example.com/test-page"); }
  std::u16string test_title() { return u"Test Page Title"; }
  std::string test_content() { return "This is test page content"; }
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
