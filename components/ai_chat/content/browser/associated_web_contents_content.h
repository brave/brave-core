/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_ASSOCIATED_WEB_CONTENTS_CONTENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_ASSOCIATED_WEB_CONTENTS_CONTENT_H_

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
}  // namespace content

class AIChatUIBrowserTest;

namespace ai_chat {
class AIChatMetrics;

// Provides context to an AI Chat conversation in the form of the Tab's content
class AssociatedWebContentsContent : public content::WebContentsObserver,
                                     public AssociatedContentDriver {
 public:
  // Delegate to extract print preview content
  class PrintPreviewExtractionDelegate {
   public:
    // Result is image data of pages or error
    using CaptureImagesCallback = base::OnceCallback<void(
        base::expected<std::vector<std::vector<uint8_t>>, std::string>)>;

    virtual ~PrintPreviewExtractionDelegate() = default;
    // Capture images of content without doing OCR
    virtual void CaptureImages(CaptureImagesCallback callback) = 0;
  };

  class PageContentFetcherDelegate {
   public:
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

  // PrintPreviewExtractionDelegate is provided as it's implementation is
  // in a different layer.
  AssociatedWebContentsContent(content::WebContents* web_contents,
                               std::unique_ptr<PrintPreviewExtractionDelegate>
                                   print_preview_extraction_delegate);

  AssociatedWebContentsContent(const AssociatedWebContentsContent&) = delete;
  AssociatedWebContentsContent& operator=(const AssociatedWebContentsContent&) =
      delete;
  ~AssociatedWebContentsContent() override;

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

  base::WeakPtr<AssociatedWebContentsContent> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  friend class ::AIChatUIBrowserTest;
  friend class AssociatedWebContentsContentUnitTest;

  // content::WebContentsObserver
  void NavigationEntryCommitted(
      const content::LoadCommittedDetails& load_details) override;
  void TitleWasSet(content::NavigationEntry* entry) override;
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;

  // ai_chat::AssociatedContentDriver
  void GetPageContent(FetchPageContentCallback callback,
                      std::string_view invalidation_token) override;
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

  void OnFetchPageContentComplete(FetchPageContentCallback callback,
                                  std::string content,
                                  bool is_video,
                                  std::string invalidation_token);

#if BUILDFLAG(ENABLE_PDF)
  void OnPDFDocumentLoadComplete(FetchPageContentCallback callback);

  void OnGetPDFPageCount(FetchPageContentCallback callback,
                         pdf::mojom::PdfListener::GetPdfBytesStatus status,
                         const std::vector<uint8_t>& bytes,
                         uint32_t page_count);

  void OnAllPDFPagesTextReceived(
      FetchPageContentCallback callback,
      const std::vector<std::pair<size_t, std::string>>& page_texts);
#endif  // BUILDFLAG(ENABLE_PDF)

  void SetPendingGetContentCallback(FetchPageContentCallback callback);

  raw_ptr<AIChatMetrics> ai_chat_metrics_;

  bool is_same_document_navigation_ = false;
  int pending_navigation_id_;
  std::u16string previous_page_title_;
  bool is_page_loaded_ = false;

  // TODO(petemill): Use signal to allow for multiple callbacks
  FetchPageContentCallback pending_get_page_content_callback_;

  std::unique_ptr<PrintPreviewExtractionDelegate>
      print_preview_extraction_delegate_;
  std::unique_ptr<PageContentFetcherDelegate> page_content_fetcher_delegate_;

  std::unique_ptr<FullScreenshotter> full_screenshotter_;

  base::WeakPtrFactory<AssociatedWebContentsContent> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_ASSOCIATED_WEB_CONTENTS_CONTENT_H_
