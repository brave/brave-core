// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"

#include <memory>

#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "brave/browser/ui/ai_chat/print_preview_extractor.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "components/favicon/core/test/mock_favicon_service.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::NiceMock;

namespace ai_chat {

class MockPrintPreviewExtractor
    : public AIChatTabHelper::PrintPreviewExtractionDelegate {
 public:
  MockPrintPreviewExtractor() = default;
  ~MockPrintPreviewExtractor() override = default;

  MOCK_METHOD(void, Extract, (bool, ExtractCallback), (override));
};

class MockPageContentFetcher
    : public AIChatTabHelper::PageContentFetcherDelegate {
 public:
  MockPageContentFetcher() = default;
  ~MockPageContentFetcher() override = default;

  MOCK_METHOD(void,
              FetchPageContent,
              (std::string_view, FetchPageContentCallback),
              (override));
  MOCK_METHOD(void,
              GetSearchSummarizerKey,
              (mojom::PageContentExtractor::GetSearchSummarizerKeyCallback),
              (override));
};

class AIChatTabHelperUnitTest : public content::RenderViewHostTestHarness {
 public:
  AIChatTabHelperUnitTest() = default;
  ~AIChatTabHelperUnitTest() override = default;

  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();
    favicon::ContentFaviconDriver::CreateForWebContents(web_contents(),
                                                        &favicon_service_);
    AIChatTabHelper::CreateForWebContents(
        web_contents(), is_print_preview_supported_
                            ? std::make_unique<MockPrintPreviewExtractor>()
                            : nullptr);
    helper_ = AIChatTabHelper::FromWebContents(web_contents());
    helper_->SetPageContentFetcherDelegateForTesting(
        std::make_unique<MockPageContentFetcher>());
    page_content_fetcher_ = static_cast<MockPageContentFetcher*>(
        helper_->GetPageContentFetcherDelegateForTesting());
    print_preview_extractor_ = static_cast<MockPrintPreviewExtractor*>(
        helper_->GetPrintPreviewExtractionDelegateForTesting());
  }

  std::unique_ptr<content::BrowserContext> CreateBrowserContext() override {
    return std::make_unique<TestingProfile>();
  }

  void NavigateTo(GURL url) {
    NavigateAndCommit(url);
    EXPECT_EQ(helper_->GetPageURL(), url);
  }

  void GetPageContent(ConversationHandler::GetPageContentCallback callback,
                      std::string_view invalidation_token) {
    helper_->GetPageContent(std::move(callback), invalidation_token);
  }

 protected:
  NiceMock<favicon::MockFaviconService> favicon_service_;
  raw_ptr<AIChatTabHelper> helper_;
  raw_ptr<MockPrintPreviewExtractor> print_preview_extractor_;
  raw_ptr<MockPageContentFetcher> page_content_fetcher_;
  bool is_print_preview_supported_ = true;
};

class AIChatTabHelperUnitTest_NoPrintPreview : public AIChatTabHelperUnitTest {
 public:
  AIChatTabHelperUnitTest_NoPrintPreview() : AIChatTabHelperUnitTest() {
    is_print_preview_supported_ = false;
  }
};

TEST_F(AIChatTabHelperUnitTest, PrintPreviewFallback_NonTriggeringURL) {
  constexpr char expected_text[] = "This is the way.";
  // A url that doesn't by itself trigger print preview extraction.
  NavigateTo(GURL("https://www.brave.com"));
  // Fallback iniatiated on empty string then succeeded.
  EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
      .WillOnce(base::test::RunOnceCallback<1>("", false, ""));
  EXPECT_CALL(*print_preview_extractor_, Extract)
      .WillOnce(base::test::RunOnceCallback<1>(expected_text));
  base::MockCallback<ConversationHandler::GetPageContentCallback> callback;
  EXPECT_CALL(callback, Run(expected_text, false, ""));
  GetPageContent(callback.Get(), "");
}

TEST_F(AIChatTabHelperUnitTest, PrintPreviewFallback_Whitespace) {
  constexpr char expected_text[] = "This is the way.";
  // A url that doesn't by itself trigger print preview extraction.
  NavigateTo(GURL("https://www.brave.com"));
  // Fallback iniatiated on white spaces and line breaks then succeeded.
  EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
      .WillOnce(
          base::test::RunOnceCallback<1>("       \n     \n  ", false, ""));
  EXPECT_CALL(*print_preview_extractor_, Extract)
      .WillOnce(base::test::RunOnceCallback<1>(expected_text));
  base::MockCallback<ConversationHandler::GetPageContentCallback> callback;
  EXPECT_CALL(callback, Run(expected_text, false, ""));
  GetPageContent(callback.Get(), "");
}

TEST_F(AIChatTabHelperUnitTest, PrintPreviewFallback_FallbackFailed) {
  // A url that doesn't by itself trigger print preview extraction.
  NavigateTo(GURL("https://www.brave.com"));
  // Fallback failed will not retrigger another fallback.
  EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
      .WillOnce(base::test::RunOnceCallback<1>("", false, ""));
  EXPECT_CALL(*print_preview_extractor_, Extract)
      .WillOnce(base::test::RunOnceCallback<1>(""));
  base::MockCallback<ConversationHandler::GetPageContentCallback> callback;
  EXPECT_CALL(callback, Run("", false, ""));
  GetPageContent(callback.Get(), "");
}

TEST_F(AIChatTabHelperUnitTest, PrintPreviewFallback_VideoContent) {
  // A url that doesn't by itself trigger print preview extraction.
  NavigateTo(GURL("https://www.brave.com"));
  // Fallback won't initiate for video content.
  EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
      .WillOnce(base::test::RunOnceCallback<1>("", true, ""));
  EXPECT_CALL(*print_preview_extractor_, Extract).Times(0);
  base::MockCallback<ConversationHandler::GetPageContentCallback> callback;
  EXPECT_CALL(callback, Run("", true, ""));
  GetPageContent(callback.Get(), "");
}

TEST_F(AIChatTabHelperUnitTest, PrintPreviewFallback_HasContent) {
  constexpr char expected_text[] = "This is the way.";
  // A url that doesn't by itself trigger print preview extraction.
  NavigateTo(GURL("https://www.brave.com"));
  // Fallback won't initiate if we already have content
  EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
      .WillOnce(base::test::RunOnceCallback<1>(expected_text, false, ""));
  EXPECT_CALL(*print_preview_extractor_, Extract).Times(0);
  base::MockCallback<ConversationHandler::GetPageContentCallback> callback;
  EXPECT_CALL(callback, Run(expected_text, false, ""));
  GetPageContent(callback.Get(), "");
}

TEST_F(AIChatTabHelperUnitTest, PrintPreviewFallback_TriggeringURL) {
  constexpr char expected_text[] = "This is the way.";
  // A url that does by itself trigger print preview extraction.
  NavigateTo(GURL("https://docs.google.com"));
  // PrintPreview always initiated on URL
  EXPECT_CALL(*page_content_fetcher_, FetchPageContent).Times(0);
  EXPECT_CALL(*print_preview_extractor_, Extract)
      .WillOnce(base::test::RunOnceCallback<1>(expected_text));
  base::MockCallback<ConversationHandler::GetPageContentCallback> callback;
  EXPECT_CALL(callback, Run(expected_text, false, ""));
  GetPageContent(callback.Get(), "");
}

TEST_F(AIChatTabHelperUnitTest, PrintPreviewFallback_TriggeringURLFailed) {
  // A url that does by itself trigger print preview extraction.
  NavigateTo(GURL("https://docs.google.com"));
  // Don't fallback to regular fetch on failed print preview extraction.
  EXPECT_CALL(*page_content_fetcher_, FetchPageContent).Times(0);
  EXPECT_CALL(*print_preview_extractor_, Extract)
      .WillOnce(base::test::RunOnceCallback<1>(""));
  base::MockCallback<ConversationHandler::GetPageContentCallback> callback;
  EXPECT_CALL(callback, Run("", false, ""));
  GetPageContent(callback.Get(), "");
}

TEST_F(AIChatTabHelperUnitTest_NoPrintPreview,
       PrintPreviewFallback_NonTriggeringURL) {
  // A url that doesn't by itself trigger print preview extraction.
  NavigateTo(GURL("https://www.brave.com"));
  // Fallback should not run (and crash) if print preview is not supported
  EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
      .WillOnce(base::test::RunOnceCallback<1>("", false, ""));
  base::MockCallback<ConversationHandler::GetPageContentCallback> callback;
  EXPECT_CALL(callback, Run("", false, ""));
  GetPageContent(callback.Get(), "");
}

TEST_F(AIChatTabHelperUnitTest_NoPrintPreview,
       PrintPreviewFallback_TriggeringURL) {
  constexpr char expected_text[] = "This is the way.";
  // A url that does by itself trigger print preview extraction.
  NavigateTo(GURL("https://docs.google.com"));
  // PrintPreview never initiated on URL (would crash)
  EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
      .WillOnce(base::test::RunOnceCallback<1>(expected_text, false, ""));
  base::MockCallback<ConversationHandler::GetPageContentCallback> callback;
  EXPECT_CALL(callback, Run(expected_text, false, ""));
  GetPageContent(callback.Get(), "");
}

}  // namespace ai_chat
