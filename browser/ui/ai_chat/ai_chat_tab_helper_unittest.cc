// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"

#include <memory>
#include <string>

#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "components/favicon/core/test/mock_favicon_service.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "content/test/test_web_contents.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
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

class MockAssociatedContentObserver : public AssociatedContentDriver::Observer {
 public:
  MockAssociatedContentObserver() = default;
  ~MockAssociatedContentObserver() override = default;

  MOCK_METHOD(void,
              OnAssociatedContentNavigated,
              (int new_navigation_id),
              (override));
};

class AIChatTabHelperUnitTest : public content::RenderViewHostTestHarness,
                                public testing::WithParamInterface<bool> {
 public:
  AIChatTabHelperUnitTest() { is_print_preview_supported_ = GetParam(); }

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
    // Verify the implementation doesn't somehow create an object for
    // PrintPreviewExtractor
    if (!is_print_preview_supported_) {
      EXPECT_EQ(print_preview_extractor_, nullptr);
    }
    observer_ = std::make_unique<NiceMock<MockAssociatedContentObserver>>();
    helper_->AddObserver(observer_.get());
  }

  void TearDown() override {
    helper_->RemoveObserver(observer_.get());
    content::RenderViewHostTestHarness::TearDown();
  }

  std::unique_ptr<content::BrowserContext> CreateBrowserContext() override {
    return std::make_unique<TestingProfile>();
  }

  void NavigateTo(GURL url,
                  bool keep_loading = false,
                  bool is_same_page = false,
                  std::string title = "") {
    if (title.empty()) {
      title = base::StrCat({url.host(), url.path()});
    }
    std::unique_ptr<content::NavigationSimulator> simulator =
        content::NavigationSimulator::CreateRendererInitiated(url, main_rfh());

    simulator->SetKeepLoading(keep_loading);

    if (is_same_page) {
      simulator->CommitSameDocument();
    } else {
      simulator->Commit();
    }
    SimulateTitleChange(base::UTF8ToUTF16(title));
    EXPECT_EQ(helper_->GetPageURL(), url);
  }

  void SimulateTitleChange(const std::u16string& title) {
    web_contents()->UpdateTitleForEntry(controller().GetLastCommittedEntry(),
                                        title);
  }

  void SimulateLoadFinished() {
    helper_->DidFinishLoad(main_rfh(), helper_->GetPageURL());
  }

  void GetPageContent(ConversationHandler::GetPageContentCallback callback,
                      std::string_view invalidation_token) {
    helper_->GetPageContent(std::move(callback), invalidation_token);
  }

  void TitleWasSet(content::NavigationEntry* entry) {
    helper_->TitleWasSet(entry);
  }

  content::TestWebContents* test_web_contents() {
    return static_cast<content::TestWebContents*>(web_contents());
  }

 protected:
  NiceMock<favicon::MockFaviconService> favicon_service_;
  std::unique_ptr<NiceMock<MockAssociatedContentObserver>> observer_;
  raw_ptr<AIChatTabHelper> helper_;
  raw_ptr<MockPrintPreviewExtractor> print_preview_extractor_;
  raw_ptr<MockPageContentFetcher> page_content_fetcher_;
  bool is_print_preview_supported_ = true;
};

INSTANTIATE_TEST_SUITE_P(
    ,
    AIChatTabHelperUnitTest,
    ::testing::Bool(),
    [](const testing::TestParamInfo<AIChatTabHelperUnitTest::ParamType>& info) {
      return base::StringPrintf("PrintPreview_%s",
                                info.param ? "Enabled" : "Disabled");
    });

TEST_P(AIChatTabHelperUnitTest, OnNewPage) {
  int current_navigation_id = -1;
  int previous_navigation_id = -2;
  // Each time we navigate, we should get call OnNewPage with a new content ID
  EXPECT_CALL(*observer_, OnAssociatedContentNavigated)
      .Times(3)
      .WillRepeatedly([&](int new_navigation_id) {
        EXPECT_GT(new_navigation_id, current_navigation_id);
        previous_navigation_id = current_navigation_id;
        current_navigation_id = new_navigation_id;
      });
  NavigateTo(GURL("https://www.brave.com"));
  NavigateTo(GURL("https://www.brave.com/1"));
  NavigateTo(GURL("https://www.brave.com/2"));

  // Going back should revive the same content id
  EXPECT_CALL(*observer_, OnAssociatedContentNavigated)
      .WillOnce([&](int new_navigation_id) {
        EXPECT_EQ(new_navigation_id, previous_navigation_id);
      });
  content::NavigationSimulator::GoBack(web_contents());

  // Same with going forward
  EXPECT_CALL(*observer_, OnAssociatedContentNavigated)
      .WillOnce([&](int new_navigation_id) {
        EXPECT_EQ(new_navigation_id, current_navigation_id);
      });
  content::NavigationSimulator::GoForward(web_contents());

  // Same-document navigation should not call OnNewPage if page title is the
  // same
  EXPECT_CALL(*observer_, OnAssociatedContentNavigated).Times(0);
  NavigateTo(GURL("https://www.brave.com/2/3"), false, true, "www.brave.com/2");
  testing::Mock::VerifyAndClearExpectations(&observer_);
  // ...unless the page title changes before the next navigation.
  EXPECT_CALL(*observer_, OnAssociatedContentNavigated)
      .WillOnce([&](int new_navigation_id) {
        EXPECT_GT(new_navigation_id, current_navigation_id);
        previous_navigation_id = current_navigation_id;
        current_navigation_id = new_navigation_id;
      });
  SimulateTitleChange(u"New Title");
  testing::Mock::VerifyAndClearExpectations(&observer_);
  // Back same-document navigation doesn't get a different title event
  // so let's check it's still detected as a new page if the navigation
  // results in a title difference.
  EXPECT_CALL(*observer_, OnAssociatedContentNavigated)
      .WillOnce([&](int new_navigation_id) {
        EXPECT_EQ(new_navigation_id, previous_navigation_id);
      });
  content::NavigationSimulator::GoBack(web_contents());
  testing::Mock::VerifyAndClearExpectations(&observer_);

  // Title changes after different-document navigation should not
  // trigger OnNewPage.
  EXPECT_CALL(*observer_, OnAssociatedContentNavigated(_)).Times(1);
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://www.brave.com/3"));
  testing::Mock::VerifyAndClearExpectations(&observer_);
  EXPECT_CALL(*observer_, OnAssociatedContentNavigated(_)).Times(0);
  SimulateTitleChange(u"Another New Title");
  testing::Mock::VerifyAndClearExpectations(&observer_);
}

TEST_P(AIChatTabHelperUnitTest, GetPageContent_HasContent) {
  constexpr char expected_text[] = "This is the way.";
  // Add whitespace to ensure it's trimmed
  constexpr char supplied_text[] = "   \n    This is the way.   \n  ";
  // A url that doesn't by itself trigger print preview extraction.
  NavigateTo(GURL("https://www.brave.com"));
  EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
      .WillOnce(base::test::RunOnceCallback<1>(supplied_text, false, ""));
  if (print_preview_extractor_) {
    // Fallback won't initiate if we already have content
    EXPECT_CALL(*print_preview_extractor_, Extract).Times(0);
  }
  base::MockCallback<ConversationHandler::GetPageContentCallback> callback;
  EXPECT_CALL(callback, Run(expected_text, false, ""));
  GetPageContent(callback.Get(), "");
}

TEST_P(AIChatTabHelperUnitTest, GetPageContent_FallbackPrintPreview) {
  constexpr char expected_text[] = "This is the way.";
  // A url that doesn't by itself trigger print preview extraction.
  NavigateTo(GURL("https://www.brave.com"));
  EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
      .WillOnce(base::test::RunOnceCallback<1>("", false, ""));
  if (print_preview_extractor_) {
    // Fallback iniatiated on empty string then succeeded.
    EXPECT_CALL(*print_preview_extractor_, Extract)
        .WillOnce(base::test::RunOnceCallback<1>(expected_text));
  }
  base::MockCallback<ConversationHandler::GetPageContentCallback> callback;
  EXPECT_CALL(callback,
              Run(print_preview_extractor_ ? expected_text : "", false, ""));
  GetPageContent(callback.Get(), "");
}

TEST_P(AIChatTabHelperUnitTest, GetPageContent_OnlyWhitespace) {
  constexpr char expected_text[] = "This is the way.";
  // A url that doesn't by itself trigger print preview extraction.
  NavigateTo(GURL("https://www.brave.com"));
  EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
      .WillOnce(
          base::test::RunOnceCallback<1>("       \n     \n  ", false, ""));
  if (print_preview_extractor_) {
    // Fallback iniatiated on white spaces and line breaks then succeeded.
    EXPECT_CALL(*print_preview_extractor_, Extract)
        .WillOnce(base::test::RunOnceCallback<1>(expected_text));
  }
  base::MockCallback<ConversationHandler::GetPageContentCallback> callback;
  EXPECT_CALL(callback,
              Run(print_preview_extractor_ ? expected_text : "", false, ""));
  GetPageContent(callback.Get(), "");
}

TEST_P(AIChatTabHelperUnitTest, GetPageContent_FallbackPrintPreviewFailed) {
  // A url that doesn't by itself trigger print preview extraction.
  NavigateTo(GURL("https://www.brave.com"));
  // Fallback failed will not retrigger another fallback.
  EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
      .WillOnce(base::test::RunOnceCallback<1>("", false, ""));
  if (print_preview_extractor_) {
    EXPECT_CALL(*print_preview_extractor_, Extract)
        .WillOnce(base::test::RunOnceCallback<1>(""));
  }
  base::MockCallback<ConversationHandler::GetPageContentCallback> callback;
  EXPECT_CALL(callback, Run("", false, ""));
  GetPageContent(callback.Get(), "");
}

TEST_P(AIChatTabHelperUnitTest, GetPageContent_VideoContent) {
  // A url that doesn't by itself trigger print preview extraction.
  NavigateTo(GURL("https://www.brave.com"));
  EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
      .WillOnce(base::test::RunOnceCallback<1>("", true, ""));
  if (print_preview_extractor_) {
    // Fallback won't initiate for video content.
    EXPECT_CALL(*print_preview_extractor_, Extract).Times(0);
  }
  base::MockCallback<ConversationHandler::GetPageContentCallback> callback;
  EXPECT_CALL(callback, Run("", true, ""));
  GetPageContent(callback.Get(), "");
}

TEST_P(AIChatTabHelperUnitTest, GetPageContent_PrintPreviewTriggeringURL) {
  base::MockCallback<ConversationHandler::GetPageContentCallback> callback;
  constexpr char expected_text[] = "This is the way.";
  // A url that does by itself trigger print preview extraction.
  NavigateTo(GURL("https://docs.google.com"));
  if (is_print_preview_supported_) {
    // PrintPreview always initiated on URL
    EXPECT_CALL(*page_content_fetcher_, FetchPageContent).Times(0);
    EXPECT_CALL(*print_preview_extractor_, Extract)
        .WillOnce(base::test::RunOnceCallback<1>(expected_text));
  } else {
    EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
        .WillOnce(base::test::RunOnceCallback<1>(expected_text, false, ""));
  }
  EXPECT_CALL(callback, Run(expected_text, false, ""));
  GetPageContent(callback.Get(), "");
}

TEST_P(AIChatTabHelperUnitTest,
       GetPageContent_PrintPreviewTriggeringURLFailed) {
  // A url that does by itself trigger print preview extraction.
  NavigateTo(GURL("https://docs.google.com"));
  // Don't fallback to regular fetch on failed print preview extraction.
  if (print_preview_extractor_) {
    EXPECT_CALL(*page_content_fetcher_, FetchPageContent).Times(0);
    EXPECT_CALL(*print_preview_extractor_, Extract)
        .WillOnce(base::test::RunOnceCallback<1>(""));
  } else {
    EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
        .WillOnce(base::test::RunOnceCallback<1>("", false, ""));
  }
  base::MockCallback<ConversationHandler::GetPageContentCallback> callback;
  EXPECT_CALL(callback, Run("", false, ""));
  GetPageContent(callback.Get(), "");
}

TEST_P(AIChatTabHelperUnitTest,
       GetPageContent_PrintPreviewTriggeringUrlWaitForLoad) {
  // A url that does by itself trigger print preview extraction.
  NavigateTo(GURL("https://docs.google.com"), /*keep_loading=*/true);
  base::MockCallback<ConversationHandler::GetPageContentCallback> callback;
  // Not epecting callback to be run until page load.
  EXPECT_CALL(callback, Run).Times(0);
  if (is_print_preview_supported_) {
    // Nothing should be called until page load
    EXPECT_CALL(*page_content_fetcher_, FetchPageContent).Times(0);
    EXPECT_CALL(*print_preview_extractor_, Extract).Times(0);
    GetPageContent(callback.Get(), "");
    testing::Mock::VerifyAndClearExpectations(&page_content_fetcher_);
    testing::Mock::VerifyAndClearExpectations(&print_preview_extractor_);
    testing::Mock::VerifyAndClearExpectations(&callback);

    // Simulate page load should trigger check again and, even with
    // empty content, callback should run.
    EXPECT_CALL(callback, Run("", false, ""));
    EXPECT_CALL(*page_content_fetcher_, FetchPageContent).Times(0);
    EXPECT_CALL(*print_preview_extractor_, Extract)
        .WillOnce(base::test::RunOnceCallback<1>(""));
    SimulateLoadFinished();

    testing::Mock::VerifyAndClearExpectations(&page_content_fetcher_);
    testing::Mock::VerifyAndClearExpectations(&print_preview_extractor_);
    testing::Mock::VerifyAndClearExpectations(&callback);
  } else {
    // FetchPageContent will not wait for page load. Let's test that the
    // re-try will wait for page load.
    EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
        .WillRepeatedly(
            base::test::RunOnceCallbackRepeatedly<1>("", false, ""));
    GetPageContent(callback.Get(), "");
    testing::Mock::VerifyAndClearExpectations(&callback);

    // Simulate page load should trigger check again and, even with
    // empty content, callback should run.
    EXPECT_CALL(callback, Run("", false, ""));
    SimulateLoadFinished();

    testing::Mock::VerifyAndClearExpectations(&page_content_fetcher_);
    testing::Mock::VerifyAndClearExpectations(&callback);
  }
}

TEST_P(AIChatTabHelperUnitTest, GetPageContent_RetryAfterLoad) {
  // A url that does not by itself trigger print preview extraction.
  NavigateTo(GURL("https://www.example.com"), /*keep_loading=*/true);
  base::MockCallback<ConversationHandler::GetPageContentCallback> callback;

  // FetchPageContent will not wait for page load. Let's test that the
  // re-try will wait for page load.
  EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
      .WillOnce(base::test::RunOnceCallback<1>("", false, ""));
  if (is_print_preview_supported_) {
    // Doesn't initialy ask for print preview extraction
    EXPECT_CALL(*print_preview_extractor_, Extract).Times(0);
  }
  EXPECT_CALL(callback, Run).Times(0);
  GetPageContent(callback.Get(), "");
  testing::Mock::VerifyAndClearExpectations(&page_content_fetcher_);
  if (is_print_preview_supported_) {
    testing::Mock::VerifyAndClearExpectations(&print_preview_extractor_);
  }
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Simulate page load should trigger check again and, even with
  // empty content, callback should run.
  const std::string expected_content = "retried content";
  if (is_print_preview_supported_) {
    // First it will try to see if after page load there is real content
    EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
        .WillOnce(base::test::RunOnceCallback<1>("", false, ""));
    // And if it has no content, it will finally try print preview extraction
    EXPECT_CALL(*print_preview_extractor_, Extract)
        .WillOnce(base::test::RunOnceCallback<1>(expected_content));
  } else {
    EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
        .WillOnce(base::test::RunOnceCallback<1>(expected_content, false, ""));
  }
  EXPECT_CALL(callback, Run(expected_content, false, ""));
  SimulateLoadFinished();

  testing::Mock::VerifyAndClearExpectations(&page_content_fetcher_);
  if (is_print_preview_supported_) {
    testing::Mock::VerifyAndClearExpectations(&print_preview_extractor_);
  }
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_P(AIChatTabHelperUnitTest,
       GetPageContent_ClearPendingCallbackOnNavigation) {
  const GURL initial_url =
      GURL(is_print_preview_supported_ ? "https://docs.google.com"
                                       : "https://www.example.com");
  for (const bool is_same_document : {false, true}) {
    SCOPED_TRACE(testing::Message() << "Same document: " << is_same_document);
    NavigateTo(initial_url,
               /*keep_loading=*/true);
    base::MockCallback<ConversationHandler::GetPageContentCallback> callback;
    EXPECT_CALL(callback, Run).Times(0);
    if (!is_print_preview_supported_) {
      EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
          .WillOnce(base::test::RunOnceCallback<1>("", false, ""));
    }
    GetPageContent(callback.Get(), "");
    testing::Mock::VerifyAndClearExpectations(&callback);

    // Navigatng should result in our pending callback being run with no content
    // and no content extraction initiated.
    EXPECT_CALL(*page_content_fetcher_, FetchPageContent).Times(0);
    if (is_print_preview_supported_) {
      EXPECT_CALL(*print_preview_extractor_, Extract).Times(0);
    }
    EXPECT_CALL(callback, Run("", false, ""));
    NavigateTo(initial_url.Resolve("/2"), /*keep_loading=*/true,
               is_same_document);
    testing::Mock::VerifyAndClearExpectations(&callback);
    testing::Mock::VerifyAndClearExpectations(&page_content_fetcher_);
    if (is_print_preview_supported_) {
      testing::Mock::VerifyAndClearExpectations(&print_preview_extractor_);
    }
  }
}

}  // namespace ai_chat
