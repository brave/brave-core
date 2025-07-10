// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"

#include <memory>
#include <string>

#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/test/test_future.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "components/favicon/core/test/mock_favicon_service.h"
#include "components/pdf/common/constants.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "content/test/test_web_contents.h"
#include "pdf/buildflags.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_PDF)
#include "components/pdf/browser/pdf_document_helper.h"
#endif  // BUILDFLAG(ENABLE_PDF)

using ::testing::_;
using ::testing::NiceMock;

namespace ai_chat {

class MockPrintPreviewExtractor
    : public AIChatTabHelper::PrintPreviewExtractionDelegate {
 public:
  MockPrintPreviewExtractor() = default;
  ~MockPrintPreviewExtractor() override = default;

  MOCK_METHOD(void, Extract, (ExtractCallback), (override));
  MOCK_METHOD(void, CapturePdf, (CapturePdfCallback), (override));
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
  MOCK_METHOD(void,
              GetOpenAIChatButtonNonce,
              (mojom::PageContentExtractor::GetOpenAIChatButtonNonceCallback),
              (override));
};

class MockAssociatedContentObserver
    : public AssociatedContentDelegate::Observer {
 public:
  MockAssociatedContentObserver() = default;
  ~MockAssociatedContentObserver() override = default;

  MOCK_METHOD(void, OnNavigated, (AssociatedContentDelegate*), (override));
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

  void GetPageContent(GetPageContentCallback callback,
                      std::string_view invalidation_token) {
    helper_->GetPageContent(std::move(callback), invalidation_token);
  }

  void TitleWasSet(content::NavigationEntry* entry) {
    helper_->TitleWasSet(entry);
  }

  content::TestWebContents* test_web_contents() {
    return static_cast<content::TestWebContents*>(web_contents());
  }

#if BUILDFLAG(ENABLE_PDF)
  void OnAllPDFPagesTextReceived(
      GetPageContentCallback callback,
      const std::vector<std::pair<size_t, std::string>>& page_texts) {
    helper_->OnAllPDFPagesTextReceived(std::move(callback), page_texts);
  }

  void OnGetPDFPageCount(GetPageContentCallback callback,
                         pdf::mojom::PdfListener::GetPdfBytesStatus status,
                         const std::vector<uint8_t>& bytes,
                         uint32_t page_count) {
    helper_->OnGetPDFPageCount(std::move(callback), status, bytes, page_count);
  }
#endif  // BUILDFLAG(ENABLE_PDF)

 protected:
  NiceMock<favicon::MockFaviconService> favicon_service_;
  std::unique_ptr<NiceMock<MockAssociatedContentObserver>> observer_;
  raw_ptr<AIChatTabHelper, DanglingUntriaged> helper_;
  raw_ptr<MockPrintPreviewExtractor, DanglingUntriaged>
      print_preview_extractor_;
  raw_ptr<MockPageContentFetcher, DanglingUntriaged> page_content_fetcher_;
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
  EXPECT_CALL(*observer_, OnNavigated).Times(3);
  NavigateTo(GURL("https://www.brave.com"));
  NavigateTo(GURL("https://www.brave.com/1"));
  NavigateTo(GURL("https://www.brave.com/2"));

  // Going back should notify navigated
  EXPECT_CALL(*observer_, OnNavigated).Times(1);
  content::NavigationSimulator::GoBack(web_contents());

  // Same with going forward
  EXPECT_CALL(*observer_, OnNavigated).Times(1);
  content::NavigationSimulator::GoForward(web_contents());

  // Same-document navigation should not call OnNewPage if page title is the
  // same
  EXPECT_CALL(*observer_, OnNavigated).Times(0);
  NavigateTo(GURL("https://www.brave.com/2/3"), false, true, "www.brave.com/2");
  testing::Mock::VerifyAndClearExpectations(&observer_);
  // ...unless the page title changes before the next navigation.
  EXPECT_CALL(*observer_, OnNavigated).Times(1);
  SimulateTitleChange(u"New Title");
  testing::Mock::VerifyAndClearExpectations(&observer_);
  // Back same-document navigation doesn't get a different title event
  // so let's check it's still detected as a new page if the navigation
  // results in a title difference.
  EXPECT_CALL(*observer_, OnNavigated).Times(1);
  content::NavigationSimulator::GoBack(web_contents());
  testing::Mock::VerifyAndClearExpectations(&observer_);

  // Title changes after different-document navigation should not
  // trigger OnNewPage.
  EXPECT_CALL(*observer_, OnNavigated(_)).Times(1);
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://www.brave.com/3"));
  testing::Mock::VerifyAndClearExpectations(&observer_);
  EXPECT_CALL(*observer_, OnNavigated(_)).Times(0);
  SimulateTitleChange(u"Another New Title");
  testing::Mock::VerifyAndClearExpectations(&observer_);
}

TEST_P(AIChatTabHelperUnitTest, GetPageContent_HasContent) {
  constexpr char kExpectedText[] = "This is the way.";
  // Add whitespace to ensure it's trimmed
  constexpr char kSuppliedText[] = "   \n    This is the way.   \n  ";
  // A url that doesn't by itself trigger print preview extraction.
  NavigateTo(GURL("https://www.brave.com"));
  EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
      .WillOnce(base::test::RunOnceCallback<1>(kSuppliedText, false, ""));
  if (print_preview_extractor_) {
    // Fallback won't initiate if we already have content
    EXPECT_CALL(*print_preview_extractor_, Extract).Times(0);
  }
  base::MockCallback<GetPageContentCallback> callback;
  EXPECT_CALL(callback, Run(kExpectedText, false, ""));
  GetPageContent(callback.Get(), "");
}

TEST_P(AIChatTabHelperUnitTest, GetPageContent_FallbackPrintPreview) {
  constexpr char kExpectedText[] = "This is the way.";
  // A url that doesn't by itself trigger print preview extraction.
  NavigateTo(GURL("https://www.brave.com"));
  EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
      .WillOnce(base::test::RunOnceCallback<1>("", false, ""));
  if (print_preview_extractor_) {
    // Fallback iniatiated on empty string then succeeded.
    EXPECT_CALL(*print_preview_extractor_, Extract)
        .WillOnce(base::test::RunOnceCallback<0>(base::ok(kExpectedText)));
  }
  base::MockCallback<GetPageContentCallback> callback;
  EXPECT_CALL(callback,
              Run(print_preview_extractor_ ? kExpectedText : "", false, ""));
  GetPageContent(callback.Get(), "");
}

TEST_P(AIChatTabHelperUnitTest, GetPageContent_OnlyWhitespace) {
  constexpr char kExpectedText[] = "This is the way.";
  // A url that doesn't by itself trigger print preview extraction.
  NavigateTo(GURL("https://www.brave.com"));
  EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
      .WillOnce(
          base::test::RunOnceCallback<1>("       \n     \n  ", false, ""));
  if (print_preview_extractor_) {
    // Fallback iniatiated on white spaces and line breaks then succeeded.
    EXPECT_CALL(*print_preview_extractor_, Extract)
        .WillOnce(base::test::RunOnceCallback<0>(base::ok(kExpectedText)));
  }
  base::MockCallback<GetPageContentCallback> callback;
  EXPECT_CALL(callback,
              Run(print_preview_extractor_ ? kExpectedText : "", false, ""));
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
        .WillOnce(base::test::RunOnceCallback<0>(base::unexpected("")));
  }
  base::MockCallback<GetPageContentCallback> callback;
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
  base::MockCallback<GetPageContentCallback> callback;
  EXPECT_CALL(callback, Run("", true, ""));
  GetPageContent(callback.Get(), "");
}

TEST_P(AIChatTabHelperUnitTest, GetPageContent_PrintPreviewTriggeringURL) {
  base::MockCallback<GetPageContentCallback> callback;
  constexpr char kExpectedText[] = "This is the way.";
  // A url that does by itself trigger print preview extraction.
  for (const auto& host : kPrintPreviewRetrievalHosts) {
    NavigateTo(GURL(base::StrCat({"https://", host})));
    if (is_print_preview_supported_) {
      // PrintPreview always initiated on URL
      EXPECT_CALL(*page_content_fetcher_, FetchPageContent).Times(0);
      EXPECT_CALL(*print_preview_extractor_, Extract)
          .WillOnce(base::test::RunOnceCallback<0>(base::ok(kExpectedText)));
    } else {
      EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
          .WillOnce(base::test::RunOnceCallback<1>(kExpectedText, false, ""));
    }
    EXPECT_CALL(callback, Run(kExpectedText, false, ""));
    GetPageContent(callback.Get(), "");
  }
}

TEST_P(AIChatTabHelperUnitTest,
       GetPageContent_PrintPreviewTriggeringURLFailed) {
  // A url that does by itself trigger print preview extraction.
  NavigateTo(GURL("https://docs.google.com"));
  // Don't fallback to regular fetch on failed print preview extraction.
  if (print_preview_extractor_) {
    EXPECT_CALL(*page_content_fetcher_, FetchPageContent).Times(0);
    EXPECT_CALL(*print_preview_extractor_, Extract)
        .WillOnce(base::test::RunOnceCallback<0>(base::unexpected("")));
  } else {
    EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
        .WillOnce(base::test::RunOnceCallback<1>("", false, ""));
  }
  base::MockCallback<GetPageContentCallback> callback;
  EXPECT_CALL(callback, Run("", false, ""));
  GetPageContent(callback.Get(), "");
}

TEST_P(AIChatTabHelperUnitTest,
       GetPageContent_PrintPreviewTriggeringUrlWaitForLoad) {
  // A url that does by itself trigger print preview extraction.
  NavigateTo(GURL("https://docs.google.com"), /*keep_loading=*/true);
  base::MockCallback<GetPageContentCallback> callback;
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
        .WillOnce(base::test::RunOnceCallback<0>(base::unexpected("")));
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
  base::MockCallback<GetPageContentCallback> callback;

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
        .WillOnce(base::test::RunOnceCallback<0>(base::ok(expected_content)));
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
    base::MockCallback<GetPageContentCallback> callback;
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

#if BUILDFLAG(ENABLE_PDF)
TEST_P(AIChatTabHelperUnitTest, OnAllPDFPagesTextReceived) {
  // Create test data with out-of-order pages
  std::vector<std::pair<size_t, std::string>> page_texts = {
      {2, "Page 3 content"},
      {0, "Page 1 content"},
      {1, "Page 2 content"},
  };

  base::test::TestFuture<std::string, bool, std::string> future;
  OnAllPDFPagesTextReceived(future.GetCallback(), page_texts);

  auto [content, is_video, invalidation_token] = future.Get();
  EXPECT_FALSE(is_video);
  EXPECT_TRUE(invalidation_token.empty());
  EXPECT_EQ(content, "Page 1 content\nPage 2 content\nPage 3 content");
}

TEST_P(AIChatTabHelperUnitTest, OnGetPDFPageCount_FailedStatus) {
  base::test::TestFuture<std::string, bool, std::string> future;

  OnGetPDFPageCount(future.GetCallback(),
                    pdf::mojom::PdfListener::GetPdfBytesStatus::kFailed,
                    std::vector<uint8_t>(), 0);

  auto [content, is_video, invalidation_token] = future.Get();
  EXPECT_TRUE(content.empty());
  EXPECT_FALSE(is_video);
  EXPECT_TRUE(invalidation_token.empty());
}

TEST_P(AIChatTabHelperUnitTest, OnGetPDFPageCount_SuccessWhenNoPDFHelper) {
  ASSERT_FALSE(pdf::PDFDocumentHelper::MaybeGetForWebContents(web_contents()));

  base::test::TestFuture<std::string, bool, std::string> future;

  OnGetPDFPageCount(future.GetCallback(),
                    pdf::mojom::PdfListener::GetPdfBytesStatus::kSuccess,
                    std::vector<uint8_t>(), 3);

  auto [content, is_video, invalidation_token] = future.Get();
  EXPECT_TRUE(content.empty());
  EXPECT_FALSE(is_video);
  EXPECT_TRUE(invalidation_token.empty());
}
#endif  // BUILDFLAG(ENABLE_PDF)

TEST_P(AIChatTabHelperUnitTest, GetPageContent_NoFallbackWhenNotPDF) {
  NavigateTo(GURL("https://www.brave.com"));
#if BUILDFLAG(ENABLE_PDF)
  ASSERT_FALSE(pdf::PDFDocumentHelper::MaybeGetForWebContents(web_contents()));
#endif  // BUILDFLAG(ENABLE_PDF)

  content::WebContentsTester::For(web_contents())
      ->SetMainFrameMimeType("text/html");

  EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
      .WillOnce(base::test::RunOnceCallback<1>("HTML content", false, ""));

  base::test::TestFuture<std::string, bool, std::string> future;
  GetPageContent(future.GetCallback(), "");

  auto [content, is_video, invalidation_token] = future.Get();
  EXPECT_EQ(content, "HTML content");
  EXPECT_FALSE(is_video);
  EXPECT_TRUE(invalidation_token.empty());

  testing::Mock::VerifyAndClearExpectations(&page_content_fetcher_);
}

TEST_P(AIChatTabHelperUnitTest,
       GetPageContent_FallbackToPrintPreviewWhenNoPDFHelper) {
  NavigateTo(GURL("https://www.brave.com"));
  content::WebContentsTester::For(web_contents())
      ->SetMainFrameMimeType(pdf::kPDFMimeType);
#if BUILDFLAG(ENABLE_PDF)
  ASSERT_FALSE(pdf::PDFDocumentHelper::MaybeGetForWebContents(web_contents()));
#endif  // BUILDFLAG(ENABLE_PDF)

  const std::string expected_text =
      is_print_preview_supported_ ? "PDF content from print preview" : "";
  base::test::TestFuture<std::string, bool, std::string> future;

  if (is_print_preview_supported_) {
    EXPECT_CALL(*page_content_fetcher_, FetchPageContent).Times(0);
    EXPECT_CALL(*print_preview_extractor_, Extract)
        .WillOnce(base::test::RunOnceCallback<0>(base::ok(expected_text)));
  } else {
    EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
        .WillOnce(base::test::RunOnceCallback<1>(expected_text, false, ""));
  }

  GetPageContent(future.GetCallback(), "");

  auto [content, is_video, invalidation_token] = future.Get();
  EXPECT_EQ(content, expected_text);
  EXPECT_FALSE(is_video);
  EXPECT_TRUE(invalidation_token.empty());

  testing::Mock::VerifyAndClearExpectations(&page_content_fetcher_);
  if (is_print_preview_supported_) {
    testing::Mock::VerifyAndClearExpectations(&print_preview_extractor_);
  }
}

}  // namespace ai_chat
