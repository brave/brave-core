/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_TAB_HELPER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_TAB_HELPER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/content/browser/full_screenshotter.h"
#include "brave/components/ai_chat/core/browser/associated_content_driver.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "pdf/buildflags.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_PDF)
#include "pdf/mojom/pdf.mojom.h"
#endif  // BUILDFLAG(ENABLE_PDF)

namespace content {
class NavigationEntry;
class RenderFrameHost;
class WebContents;
struct LoadCommittedDetails;
}

class AIChatUIBrowserTest;

namespace ai_chat {
class AIChatMetrics;

// Provides context to an AI Chat conversation in the form of the Tab's content
class AIChatTabHelper : public content::WebContentsObserver,
                        public content::WebContentsUserData<AIChatTabHelper>,
                        public AssociatedContentDriver {
 public:
  using GetPageContentCallback = GetPageContentCallback;

  // Delegate to extract print preview content
  class PrintPreviewExtractionDelegate {
   public:
    // Result is extracted text or error
    using ExtractCallback =
        base::OnceCallback<void(base::expected<std::string, std::string>)>;
    // Result is image data of pdf pages or error
    using CapturePdfCallback = base::OnceCallback<void(
        base::expected<std::vector<std::vector<uint8_t>>, std::string>)>;

    virtual ~PrintPreviewExtractionDelegate() = default;
    // Get the current text from the WebContents using Print Preview and OCR
    virtual void Extract(ExtractCallback callback) = 0;
    // Capture images of pdf without doing OCR
    virtual void CapturePdf(CapturePdfCallback callback) = 0;
  };

  class PageContentFetcherDelegate {
   public:
    using FetchPageContentCallback =
        base::OnceCallback<void(std::string page_content,
                                bool is_video,
                                std::string invalidation_token)>;

    virtual ~PageContentFetcherDelegate() = default;

    // Gets text of the page content, making an attempt
    // to only consider the main content of the page.
    virtual void FetchPageContent(std::string_view invalidation_token,
                                  FetchPageContentCallback callback) = 0;

    // Attempts to find a search summarizer key for the page.
    virtual void GetSearchSummarizerKey(
        GetSearchSummarizerKeyCallback callback) = 0;

    // Fetches the nonce for the OpenLeo button from the page HTML and validate
    // if it matches the href URL and the passed in nonce.
    virtual void GetOpenAIChatButtonNonce(
        mojom::PageContentExtractor::GetOpenAIChatButtonNonceCallback
            callback) = 0;
  };

  AIChatTabHelper(const AIChatTabHelper&) = delete;
  AIChatTabHelper& operator=(const AIChatTabHelper&) = delete;
  ~AIChatTabHelper() override;

  void SetPageContentFetcherDelegateForTesting(
      std::unique_ptr<PageContentFetcherDelegate> delegate) {
    page_content_fetcher_delegate_ = std::move(delegate);
  }
  raw_ptr<PageContentFetcherDelegate>
  GetPageContentFetcherDelegateForTesting() {
    return page_content_fetcher_delegate_.get();
  }
  raw_ptr<PrintPreviewExtractionDelegate>
  GetPrintPreviewExtractionDelegateForTesting() {
    return print_preview_extraction_delegate_.get();
  }

  void GetOpenAIChatButtonNonce(
      mojom::PageContentExtractor::GetOpenAIChatButtonNonceCallback callback);

 private:
  friend class content::WebContentsUserData<AIChatTabHelper>;
  friend class ::AIChatUIBrowserTest;
  friend class AIChatTabHelperUnitTest;

  // PrintPreviewExtractionDelegate is provided as it's implementation is
  // in a different layer.
  AIChatTabHelper(content::WebContents* web_contents,
                  std::unique_ptr<PrintPreviewExtractionDelegate>
                      print_preview_extraction_delegate);

  // content::WebContentsObserver
  void WebContentsDestroyed() override;
  void NavigationEntryCommitted(
      const content::LoadCommittedDetails& load_details) override;
  void TitleWasSet(content::NavigationEntry* entry) override;
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;

  // ai_chat::AssociatedContentDriver
  GURL GetPageURL() const override;
  void GetPageContent(GetPageContentCallback callback,
                      std::string_view invalidation_token) override;
  std::u16string GetPageTitle() const override;
  void OnNewPage(int64_t navigation_id) override;

  // Called when an event of significance occurs that, if the page is a
  // same-document navigation, should result in that previous navigation
  // being considered as a new page.
  void MaybeSameDocumentIsNewPage();

  void GetSearchSummarizerKey(GetSearchSummarizerKeyCallback callback) override;

  bool HasOpenAIChatPermission() const override;

  void GetScreenshots(
      mojom::ConversationHandler::GetScreenshotsCallback callback) override;

  void OnScreenshotsCaptured(
      mojom::ConversationHandler::GetScreenshotsCallback callback,
      base::expected<std::vector<std::vector<uint8_t>>, std::string>);

  void OnFetchPageContentComplete(GetPageContentCallback callback,
                                  std::string content,
                                  bool is_video,
                                  std::string invalidation_token);

  void OnExtractPrintPreviewContentComplete(
      GetPageContentCallback callback,
      base::expected<std::string, std::string>);

#if BUILDFLAG(ENABLE_PDF)
  void OnPDFDocumentLoadComplete(GetPageContentCallback callback);

  void OnGetPDFPageCount(GetPageContentCallback callback,
                         pdf::mojom::PdfListener::GetPdfBytesStatus status,
                         const std::vector<uint8_t>& bytes,
                         uint32_t page_count);

  void OnAllPDFPagesTextReceived(
      GetPageContentCallback callback,
      const std::vector<std::pair<size_t, std::string>>& page_texts);
#endif  // BUILDFLAG(ENABLE_PDF)

  bool MaybePrintPreviewExtract(GetPageContentCallback& callback);

  void SetPendingGetContentCallback(GetPageContentCallback callback);

  raw_ptr<AIChatMetrics> ai_chat_metrics_;

  bool is_same_document_navigation_ = false;
  int pending_navigation_id_;
  std::u16string previous_page_title_;
  bool is_page_loaded_ = false;

  // TODO(petemill): Use signal to allow for multiple callbacks
  GetPageContentCallback pending_get_page_content_callback_;

  std::unique_ptr<PrintPreviewExtractionDelegate>
      print_preview_extraction_delegate_;
  std::unique_ptr<PageContentFetcherDelegate> page_content_fetcher_delegate_;

  std::unique_ptr<FullScreenshotter> full_screenshotter_;

  base::WeakPtrFactory<AIChatTabHelper> weak_ptr_factory_{this};
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_TAB_HELPER_H_
