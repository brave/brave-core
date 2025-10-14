// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/associated_web_contents.h"

#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/test/test_future.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
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
#include "third_party/abseil-cpp/absl/strings/str_format.h"

#if BUILDFLAG(ENABLE_PDF)
#include "components/pdf/browser/pdf_document_helper.h"
#endif  // BUILDFLAG(ENABLE_PDF)

using ::testing::_;
using ::testing::NiceMock;

namespace ai_chat {

class MockPrintPreviewExtractor
    : public AssociatedWebContents::PrintPreviewExtractionDelegate {
 public:
  MockPrintPreviewExtractor() = default;
  ~MockPrintPreviewExtractor() override = default;

  MOCK_METHOD(void, CaptureImages, (CaptureImagesCallback), (override));
};

class MockPageContentFetcher
    : public AssociatedWebContents::PageContentFetcherDelegate {
 public:
  MockPageContentFetcher() = default;
  ~MockPageContentFetcher() override = default;

  MOCK_METHOD(void,
              FetchPageContent,
              (std::string_view,
               AssociatedWebContents::FetchPageContentCallback),
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

  MOCK_METHOD(void, OnRequestArchive, (AssociatedContentDelegate*), (override));
};

class AssociatedWebContentsUnitTest : public content::RenderViewHostTestHarness,
                                      public testing::WithParamInterface<bool> {
 public:
  AssociatedWebContentsUnitTest() { is_print_preview_supported_ = GetParam(); }

  ~AssociatedWebContentsUnitTest() override = default;

  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();
    favicon::ContentFaviconDriver::CreateForWebContents(web_contents(),
                                                        &favicon_service_);
    AIChatTabHelper::CreateForWebContents(
        web_contents(), is_print_preview_supported_
                            ? std::make_unique<MockPrintPreviewExtractor>()
                            : nullptr);
    auto* helper = AIChatTabHelper::FromWebContents(web_contents());
    associated_web_contents_ = helper->associated_web_contents();

    associated_web_contents_->SetPageContentFetcherDelegateForTesting(
        std::make_unique<MockPageContentFetcher>());
    page_content_fetcher_ = static_cast<MockPageContentFetcher*>(
        associated_web_contents_->GetPageContentFetcherDelegateForTesting());
    print_preview_extractor_ = static_cast<MockPrintPreviewExtractor*>(
        associated_web_contents_
            ->GetPrintPreviewExtractionDelegateForTesting());
    // Verify the implementation doesn't somehow create an object for
    // PrintPreviewExtractor
    if (!is_print_preview_supported_) {
      EXPECT_EQ(print_preview_extractor_, nullptr);
    }
    observer_ = std::make_unique<NiceMock<MockAssociatedContentObserver>>();
    associated_web_contents_->AddObserver(observer_.get());
  }

  void TearDown() override {
    associated_web_contents_->RemoveObserver(observer_.get());
    associated_web_contents_ = nullptr;

    content::RenderViewHostTestHarness::TearDown();
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
    EXPECT_EQ(web_contents()->GetLastCommittedURL(), url);
  }

  void SimulateTitleChange(const std::u16string& title) {
    web_contents()->UpdateTitleForEntry(controller().GetLastCommittedEntry(),
                                        title);
  }

  void SimulateLoadFinished() {
    associated_web_contents_->DidFinishLoad(
        main_rfh(), web_contents()->GetLastCommittedURL());
  }

  void GetPageContent(AssociatedWebContents::FetchPageContentCallback callback,
                      std::string_view invalidation_token) {
    associated_web_contents_->GetPageContent(std::move(callback),
                                             invalidation_token);
  }

  void GetScreenshots(
      mojom::ConversationHandler::GetScreenshotsCallback callback) {
    associated_web_contents_->GetScreenshots(std::move(callback));
  }

  void TitleWasSet(content::NavigationEntry* entry) {
    associated_web_contents_->TitleWasSet(entry);
  }

  content::TestWebContents* test_web_contents() {
    return static_cast<content::TestWebContents*>(web_contents());
  }

#if BUILDFLAG(ENABLE_PDF)
  void OnAllPDFPagesTextReceived(
      AssociatedWebContents::FetchPageContentCallback callback,
      const std::vector<std::pair<size_t, std::string>>& page_texts) {
    associated_web_contents_->OnAllPDFPagesTextReceived(std::move(callback),
                                                        page_texts);
  }

  void OnGetPDFPageCount(
      AssociatedWebContents::FetchPageContentCallback callback,
      pdf::mojom::PdfListener::GetPdfBytesStatus status,
      const std::vector<uint8_t>& bytes,
      uint32_t page_count) {
    associated_web_contents_->OnGetPDFPageCount(std::move(callback), status,
                                                bytes, page_count);
  }
#endif  // BUILDFLAG(ENABLE_PDF)

 protected:
  NiceMock<favicon::MockFaviconService> favicon_service_;
  std::unique_ptr<NiceMock<MockAssociatedContentObserver>> observer_;
  raw_ptr<AssociatedWebContents> associated_web_contents_;
  raw_ptr<MockPrintPreviewExtractor, DanglingUntriaged>
      print_preview_extractor_;
  raw_ptr<MockPageContentFetcher, DanglingUntriaged> page_content_fetcher_;
  bool is_print_preview_supported_ = true;
};

INSTANTIATE_TEST_SUITE_P(
    ,
    AssociatedWebContentsUnitTest,
    ::testing::Bool(),
    [](const testing::TestParamInfo<AssociatedWebContentsUnitTest::ParamType>&
           info) {
      return absl::StrFormat("PrintPreview_%s",
                             info.param ? "Enabled" : "Disabled");
    });

TEST_P(AssociatedWebContentsUnitTest, OnNewPage) {
  EXPECT_CALL(*observer_, OnRequestArchive).Times(3);
  NavigateTo(GURL("https://www.brave.com"));
  NavigateTo(GURL("https://www.brave.com/1"));
  NavigateTo(GURL("https://www.brave.com/2"));

  // Going back should notify navigated
  EXPECT_CALL(*observer_, OnRequestArchive).Times(1);
  content::NavigationSimulator::GoBack(web_contents());

  // Same with going forward
  EXPECT_CALL(*observer_, OnRequestArchive).Times(1);
  content::NavigationSimulator::GoForward(web_contents());

  // Same-document navigation should not call OnNewPage if page title is the
  // same
  EXPECT_CALL(*observer_, OnRequestArchive).Times(0);
  NavigateTo(GURL("https://www.brave.com/2/3"), false, true, "www.brave.com/2");
  testing::Mock::VerifyAndClearExpectations(&observer_);
  // ...unless the page title changes before the next navigation.
  EXPECT_CALL(*observer_, OnRequestArchive).Times(1);
  SimulateTitleChange(u"New Title");
  testing::Mock::VerifyAndClearExpectations(&observer_);
  // Back same-document navigation doesn't get a different title event
  // so let's check it's still detected as a new page if the navigation
  // results in a title difference.
  EXPECT_CALL(*observer_, OnRequestArchive).Times(1);
  content::NavigationSimulator::GoBack(web_contents());
  testing::Mock::VerifyAndClearExpectations(&observer_);

  // Title changes after different-document navigation should not
  // trigger OnNewPage.
  EXPECT_CALL(*observer_, OnRequestArchive(_)).Times(1);
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://www.brave.com/3"));
  testing::Mock::VerifyAndClearExpectations(&observer_);
  EXPECT_CALL(*observer_, OnRequestArchive(_)).Times(0);
  SimulateTitleChange(u"Another New Title");
  testing::Mock::VerifyAndClearExpectations(&observer_);
}

TEST_P(AssociatedWebContentsUnitTest, GetPageContent_HasContent) {
  constexpr char kExpectedText[] = "This is the way.";
  // Add whitespace to ensure it's trimmed
  constexpr char kSuppliedText[] = "   \n    This is the way.   \n  ";
  // A url that doesn't by itself trigger print preview extraction.
  NavigateTo(GURL("https://www.brave.com"));
  EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
      .WillOnce(base::test::RunOnceCallback<1>(kSuppliedText, false, ""));
  // Note: No need to mock CaptureImages since print preview hosts
  // are not triggered when regular content is available
  base::MockCallback<AssociatedWebContents::FetchPageContentCallback> callback;
  EXPECT_CALL(callback, Run(kExpectedText, false, ""));
  GetPageContent(callback.Get(), "");
}

TEST_P(AssociatedWebContentsUnitTest, GetPageContent_VideoContent) {
  // A url that doesn't by itself trigger print preview extraction.
  NavigateTo(GURL("https://www.brave.com"));
  EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
      .WillOnce(base::test::RunOnceCallback<1>("", true, ""));
  // Note: No need to mock CaptureImages since print preview hosts
  // are not triggered for video content
  base::MockCallback<AssociatedWebContents::FetchPageContentCallback> callback;
  EXPECT_CALL(callback, Run("", true, ""));
  GetPageContent(callback.Get(), "");
}

TEST_P(AssociatedWebContentsUnitTest,
       GetPageContent_PrintPreviewTriggeringURL) {
  base::MockCallback<AssociatedWebContents::FetchPageContentCallback> callback;
  // A url that triggers print preview extraction - should return empty content
  // to allow autoscreenshots mechanism to handle server-side OCR
  for (const auto& host : kPrintPreviewRetrievalHosts) {
    NavigateTo(GURL(base::StrCat({"https://", host})));
    if (is_print_preview_supported_) {
      // PrintPreview returns empty content to trigger autoscreenshots
      EXPECT_CALL(*page_content_fetcher_, FetchPageContent).Times(0);
      // No Extract call - we now return empty to trigger autoscreenshots
    } else {
      EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
          .WillOnce(base::test::RunOnceCallback<1>("", false, ""));
    }
    // Expect empty content which will trigger autoscreenshots in real usage
    EXPECT_CALL(callback, Run("", false, ""));
    GetPageContent(callback.Get(), "");
  }
}

TEST_P(AssociatedWebContentsUnitTest,
       GetPageContent_PrintPreviewTriggeringURLFailed) {
  // A url that does by itself trigger print preview extraction.
  NavigateTo(GURL("https://docs.google.com"));
  // Don't fallback to regular fetch on failed print preview extraction.
  if (print_preview_extractor_) {
    EXPECT_CALL(*page_content_fetcher_, FetchPageContent).Times(0);
    // Print preview now returns empty content directly
  } else {
    EXPECT_CALL(*page_content_fetcher_, FetchPageContent)
        .WillOnce(base::test::RunOnceCallback<1>("", false, ""));
  }
  base::MockCallback<AssociatedWebContents::FetchPageContentCallback> callback;
  EXPECT_CALL(callback, Run("", false, ""));
  GetPageContent(callback.Get(), "");
}

TEST_P(AssociatedWebContentsUnitTest,
       GetPageContent_PrintPreviewTriggeringUrlWaitForLoad) {
  // A url that does by itself trigger print preview extraction.
  NavigateTo(GURL("https://docs.google.com"), /*keep_loading=*/true);
  base::MockCallback<AssociatedWebContents::FetchPageContentCallback> callback;
  // Not epecting callback to be run until page load.
  EXPECT_CALL(callback, Run).Times(0);
  if (is_print_preview_supported_) {
    // Nothing should be called until page load
    EXPECT_CALL(*page_content_fetcher_, FetchPageContent).Times(0);
    // No CaptureImages calls expected until page loads
    GetPageContent(callback.Get(), "");
    testing::Mock::VerifyAndClearExpectations(&page_content_fetcher_);
    testing::Mock::VerifyAndClearExpectations(&print_preview_extractor_);
    testing::Mock::VerifyAndClearExpectations(&callback);

    // Simulate page load should trigger check again and, even with
    // empty content, callback should run.
    EXPECT_CALL(callback, Run("", false, ""));
    EXPECT_CALL(*page_content_fetcher_, FetchPageContent).Times(0);
    // Print preview now returns empty content directly
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

TEST_P(AssociatedWebContentsUnitTest,
       GetPageContent_ClearPendingCallbackOnNavigation) {
  const GURL initial_url =
      GURL(is_print_preview_supported_ ? "https://docs.google.com"
                                       : "https://www.example.com");
  for (const bool is_same_document : {false, true}) {
    SCOPED_TRACE(testing::Message() << "Same document: " << is_same_document);
    NavigateTo(initial_url,
               /*keep_loading=*/true);
    base::MockCallback<AssociatedWebContents::FetchPageContentCallback>
        callback;
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
    // No CaptureImages calls expected during navigation
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
TEST_P(AssociatedWebContentsUnitTest, OnAllPDFPagesTextReceived) {
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

TEST_P(AssociatedWebContentsUnitTest, OnGetPDFPageCount_FailedStatus) {
  base::test::TestFuture<std::string, bool, std::string> future;

  OnGetPDFPageCount(future.GetCallback(),
                    pdf::mojom::PdfListener::GetPdfBytesStatus::kFailed,
                    std::vector<uint8_t>(), 0);

  auto [content, is_video, invalidation_token] = future.Get();
  EXPECT_TRUE(content.empty());
  EXPECT_FALSE(is_video);
  EXPECT_TRUE(invalidation_token.empty());
}

TEST_P(AssociatedWebContentsUnitTest,
       OnGetPDFPageCount_SuccessWhenNoPDFHelper) {
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

TEST_P(AssociatedWebContentsUnitTest, GetPageContent_NoFallbackWhenNotPDF) {
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

// Tests for GetScreenshots method decision logic
TEST_P(AssociatedWebContentsUnitTest, GetScreenshots_PrintPreviewHost) {
  // Test that print preview hosts use CaptureImages when delegate is available
  NavigateTo(GURL("https://docs.google.com/document"));

  base::test::TestFuture<std::optional<std::vector<mojom::UploadedFilePtr>>>
      future;

  if (is_print_preview_supported_) {
    // Should use print preview extraction
    EXPECT_CALL(*print_preview_extractor_, CaptureImages)
        .WillOnce(base::test::RunOnceCallback<0>(
            base::expected<std::vector<std::vector<uint8_t>>, std::string>(
                std::vector<std::vector<uint8_t>>{{0x89, 0x50, 0x4E, 0x47}})));
  }

  GetScreenshots(future.GetCallback());

  auto result = future.Take();
  if (is_print_preview_supported_) {
    // Should receive screenshots when print preview is supported
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result->empty());
  } else {
    // When print preview is not supported, should return empty result
    EXPECT_FALSE(result.has_value());
  }
}

TEST_P(AssociatedWebContentsUnitTest, GetScreenshots_RegularHost) {
  // Test that regular hosts use FullScreenshotter (we can't easily mock this)
  NavigateTo(GURL("https://www.example.com"));

  base::test::TestFuture<std::optional<std::vector<mojom::UploadedFilePtr>>>
      future;

  if (is_print_preview_supported_) {
    // Print preview extractor should NOT be called for regular hosts
    EXPECT_CALL(*print_preview_extractor_, CaptureImages).Times(0);
  }

  // Note: We can't easily mock FullScreenshotter since it's created internally,
  // but we can verify that CaptureImages is not called on the print preview
  // extractor
  GetScreenshots(future.GetCallback());

  // The result should be provided by FullScreenshotter
  auto result = future.Take();
  // We can't predict the exact result since FullScreenshotter behavior
  // depends on the actual rendering, but we can verify the call completed
}

TEST_P(AssociatedWebContentsUnitTest, GetScreenshots_MultipleHosts) {
  // Test all print preview hosts
  for (const auto& host : kPrintPreviewRetrievalHosts) {
    SCOPED_TRACE(testing::Message() << "Testing host: " << host);
    NavigateTo(GURL(base::StrCat({"https://", host, "/document"})));

    base::test::TestFuture<std::optional<std::vector<mojom::UploadedFilePtr>>>
        future;

    if (is_print_preview_supported_) {
      // Should use print preview extraction for all print preview hosts
      EXPECT_CALL(*print_preview_extractor_, CaptureImages)
          .WillOnce(base::test::RunOnceCallback<0>(
              base::expected<std::vector<std::vector<uint8_t>>, std::string>(
                  std::vector<std::vector<uint8_t>>{
                      {0x89, 0x50, 0x4E, 0x47}})));
    }

    GetScreenshots(future.GetCallback());

    auto result = future.Take();
    if (is_print_preview_supported_) {
      EXPECT_TRUE(result.has_value());
      EXPECT_FALSE(result->empty());
    } else {
      EXPECT_FALSE(result.has_value());
    }

    if (is_print_preview_supported_) {
      testing::Mock::VerifyAndClearExpectations(&print_preview_extractor_);
    }
  }
}

TEST_P(AssociatedWebContentsUnitTest, GetScreenshots_PrintPreviewError) {
  // Test error handling when print preview extraction fails
  NavigateTo(GURL("https://docs.google.com/document"));

  base::test::TestFuture<std::optional<std::vector<mojom::UploadedFilePtr>>>
      future;

  if (is_print_preview_supported_) {
    // Simulate print preview extraction error
    EXPECT_CALL(*print_preview_extractor_, CaptureImages)
        .WillOnce(base::test::RunOnceCallback<0>(
            base::unexpected<std::string>("Print preview extraction failed")));
  }

  GetScreenshots(future.GetCallback());

  auto result = future.Take();
  // Should return empty result on error or when not supported
  EXPECT_FALSE(result.has_value());
}

#if BUILDFLAG(ENABLE_PDF)
TEST_P(AssociatedWebContentsUnitTest, GetScreenshots_PDFContent) {
  // Test that PDF content uses print preview extraction
  NavigateTo(GURL("https://example.com/document.pdf"));

  // Set the main frame MIME type to PDF
  content::WebContentsTester::For(web_contents())
      ->SetMainFrameMimeType(pdf::kPDFMimeType);

  base::test::TestFuture<std::optional<std::vector<mojom::UploadedFilePtr>>>
      future;

  if (is_print_preview_supported_) {
    // Should use print preview extraction for PDFs even on non-print-preview
    // hosts
    EXPECT_CALL(*print_preview_extractor_, CaptureImages)
        .WillOnce(base::test::RunOnceCallback<0>(
            base::expected<std::vector<std::vector<uint8_t>>, std::string>(
                std::vector<std::vector<uint8_t>>{{0x25, 0x50, 0x44, 0x46}})));
  }

  GetScreenshots(future.GetCallback());

  auto result = future.Take();
  if (is_print_preview_supported_) {
    // When print preview is supported, should receive screenshots from print
    // preview extraction
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result->empty());
  }
  // When print preview is not supported, PDFs fall back to FullScreenshotter
  // We can't predict the exact FullScreenshotter result, but the call should
  // complete
}
#endif  // BUILDFLAG(ENABLE_PDF)

}  // namespace ai_chat
