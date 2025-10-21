/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/content/browser/associated_web_contents_content.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "base/barrier_callback.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_ostream_operators.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/ai_chat/content/browser/page_content_fetcher.h"
#include "brave/components/ai_chat/content/browser/pdf_utils.h"
#include "brave/components/ai_chat/core/browser/associated_content_driver.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "content/public/browser/browser_accessibility_state.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/permission_controller.h"
#include "content/public/browser/permission_descriptor_util.h"
#include "content/public/browser/permission_result.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "pdf/buildflags.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"

#if BUILDFLAG(ENABLE_PDF)
#include "components/pdf/browser/pdf_document_helper.h"
#endif  // BUILDFLAG(ENABLE_PDF)

namespace ai_chat {

AssociatedWebContentsContent::AssociatedWebContentsContent(
    content::WebContents* web_contents,
    std::unique_ptr<PrintPreviewExtractionDelegate>
        print_preview_extraction_delegate)
    : content::WebContentsObserver(web_contents),
      AssociatedContentDriver(web_contents->GetBrowserContext()
                                  ->GetDefaultStoragePartition()
                                  ->GetURLLoaderFactoryForBrowserProcess()),
      print_preview_extraction_delegate_(
          std::move(print_preview_extraction_delegate)),
      page_content_fetcher_delegate_(
          std::make_unique<PageContentFetcher>(web_contents)) {
  previous_page_title_ = web_contents->GetTitle();
}

AssociatedWebContentsContent::~AssociatedWebContentsContent() = default;

void AssociatedWebContentsContent::NavigationEntryCommitted(
    const content::LoadCommittedDetails& load_details) {
  if (!load_details.is_main_frame) {
    return;
  }
  // UniqueID will provide a consistent value for the entry when navigating
  // through history, allowing us to re-join conversations and navigations.
  int pending_navigation_id = load_details.entry->GetUniqueID();
  pending_navigation_id_ = pending_navigation_id;
  DVLOG(2) << __func__ << " id: " << pending_navigation_id_
           << "\n url: " << load_details.entry->GetVirtualURL()
           << "\n current page title: " << load_details.entry->GetTitle()
           << "\n previous page title: " << previous_page_title_
           << "\n same document? " << load_details.is_same_document;

  // Allow same-document navigation, as content often changes as a result
  // of framgment / pushState / replaceState navigations.
  // Content won't be retrieved immediately and we don't have a similar
  // "DOM Content Loaded" event, so let's wait for something else such as
  // page title changing before committing to starting a new conversation
  // and treating it as a "fresh page".
  is_same_document_navigation_ = load_details.is_same_document;
  // Experimentally only call |OnNewPage| for same-page navigations _if_
  // it results in a page title change (see |TtileWasSet|). Title detection
  // also done within the navigation entry so that back/forward navigations
  // are handled correctly.

  // Page loaded is only considered changing when full document changes
  if (!is_same_document_navigation_) {
    is_page_loaded_ = false;
  }
  if (!is_same_document_navigation_ ||
      previous_page_title_ != load_details.entry->GetTitle()) {
    OnNewPage(pending_navigation_id_);
  }
  previous_page_title_ = load_details.entry->GetTitle();
}

void AssociatedWebContentsContent::TitleWasSet(
    content::NavigationEntry* entry) {
  DVLOG(2) << __func__ << ": id=" << entry->GetUniqueID()
           << " title=" << entry->GetTitle();
  MaybeSameDocumentIsNewPage();
  previous_page_title_ = entry->GetTitle();
  SetTitle(entry->GetTitle());
}

void AssociatedWebContentsContent::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  DVLOG(4) << __func__ << ": " << validated_url.spec();
  if (validated_url == web_contents()->GetLastCommittedURL()) {
    is_page_loaded_ = true;
    if (pending_get_page_content_callback_) {
      GetPageContent(std::move(pending_get_page_content_callback_), "");
    }
  }
}

void AssociatedWebContentsContent::GetPageContent(
    FetchPageContentCallback callback,
    std::string_view invalidation_token) {
  bool is_pdf = IsPdf(web_contents());
  if (is_pdf) {
#if BUILDFLAG(ENABLE_PDF)
    auto* pdf_helper =
        pdf::PDFDocumentHelper::MaybeGetForWebContents(web_contents());
    if (pdf_helper) {
      pdf_helper->RegisterForDocumentLoadComplete(base::BindOnce(
          &AssociatedWebContentsContent::OnPDFDocumentLoadComplete,
          weak_ptr_factory_.GetWeakPtr(),
          base::BindOnce(
              &AssociatedWebContentsContent::OnFetchPageContentComplete,
              weak_ptr_factory_.GetWeakPtr(), std::move(callback))));
      return;
    }
#endif  // BUILDFLAG(ENABLE_PDF)
    // If we have a PDF but no PDFHelper there's no point running one of our
    // other extractors - we'll just end up with empty content anyway.
    std::move(callback).Run("", false, "");
    return;
  }
  if (print_preview_extraction_delegate_ &&
      kPrintPreviewRetrievalHosts.contains(
          web_contents()->GetLastCommittedURL().host_piece())) {
    // Get content using print preview image capture for server-side OCR
    DVLOG(1) << __func__ << " print preview url";
    // For print preview hosts, we always return empty content to trigger
    // the autoscreenshots mechanism which will use CaptureImages for
    // server-side OCR. However, if the page isn't loaded yet, wait for load
    // completion.
    if (!is_page_loaded_) {
      DVLOG(1) << "print preview page was not loaded yet, will return empty "
                  "after load";
      SetPendingGetContentCallback(std::move(callback));
      return;
    }
    DVLOG(1) << "print preview host detected, returning empty to trigger "
                "autoscreenshots";
    std::move(callback).Run("", false, "");
    return;
  }
  page_content_fetcher_delegate_->FetchPageContent(
      invalidation_token,
      base::BindOnce(&AssociatedWebContentsContent::OnFetchPageContentComplete,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AssociatedWebContentsContent::OnFetchPageContentComplete(
    FetchPageContentCallback callback,
    std::string content,
    bool is_video,
    std::string invalidation_token) {
  base::TrimWhitespaceASCII(content, base::TRIM_ALL, &content);
  // If content is empty, and page was not loaded yet, wait for page load.
  // Once page load is complete, try again.
  if (content.empty() && !is_video && !is_page_loaded_) {
    DVLOG(1) << "page was not loaded yet, will try again after load";
    SetPendingGetContentCallback(std::move(callback));
    return;
  }
  std::move(callback).Run(std::move(content), is_video,
                          std::move(invalidation_token));
}

void AssociatedWebContentsContent::SetPendingGetContentCallback(
    FetchPageContentCallback callback) {
  if (pending_get_page_content_callback_) {
    std::move(pending_get_page_content_callback_).Run("", false, "");
  }
  pending_get_page_content_callback_ = std::move(callback);
}

void AssociatedWebContentsContent::OnNewPage(int64_t navigation_id) {
  DVLOG(3) << __func__ << " id: " << navigation_id;
  AssociatedContentDriver::OnNewPage(navigation_id);
  set_url(web_contents()->GetLastCommittedURL());
  SetTitle(web_contents()->GetTitle());
  if (pending_get_page_content_callback_) {
    std::move(pending_get_page_content_callback_).Run("", false, "");
  }
}

void AssociatedWebContentsContent::MaybeSameDocumentIsNewPage() {
  if (is_same_document_navigation_) {
    DVLOG(2) << "Same document navigation detected new \"page\" - calling "
                "OnNewPage()";
    // Cancel knowledge that the current navigation should be associated
    // with any conversation that's associated with the previous navigation.
    // Tell any conversation that it shouldn't be associated with this
    // content anymore, as we've moved on.
    OnNewPage(pending_navigation_id_);
    // Don't respond to further TitleWasSet
    is_same_document_navigation_ = false;
  }
}

#if BUILDFLAG(ENABLE_PDF)
void AssociatedWebContentsContent::OnPDFDocumentLoadComplete(
    FetchPageContentCallback callback) {
  auto* pdf_helper =
      pdf::PDFDocumentHelper::MaybeGetForWebContents(web_contents());
  if (!pdf_helper) {
    std::move(callback).Run("", false, "");
    return;
  }

  // Fetch zero PDF bytes to just receive the total page count.
  pdf_helper->GetPdfBytes(
      /*size_limit=*/0,
      base::BindOnce(&AssociatedWebContentsContent::OnGetPDFPageCount,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AssociatedWebContentsContent::OnGetPDFPageCount(
    FetchPageContentCallback callback,
    pdf::mojom::PdfListener::GetPdfBytesStatus status,
    const std::vector<uint8_t>& bytes,
    uint32_t page_count) {
  auto* pdf_helper =
      pdf::PDFDocumentHelper::MaybeGetForWebContents(web_contents());
  if (status == pdf::mojom::PdfListener::GetPdfBytesStatus::kFailed ||
      !pdf_helper) {
    std::move(callback).Run("", false, "");
    return;
  }

  // Create a barrier callback that will be called when all pages are received
  auto barrier_callback = base::BarrierCallback<std::pair<size_t, std::string>>(
      page_count,
      base::BindOnce(&AssociatedWebContentsContent::OnAllPDFPagesTextReceived,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));

  for (size_t i = 0; i < page_count; ++i) {
    pdf_helper->GetPageText(
        i, base::BindOnce(
               [](base::OnceCallback<void(std::pair<size_t, std::string>)>
                      barrier_callback,
                  size_t page_index, const std::u16string& page_text) {
                 // Convert to UTF8 immediately and include page index
                 std::move(barrier_callback)
                     .Run(std::make_pair(page_index,
                                         base::UTF16ToUTF8(page_text)));
               },
               barrier_callback, i));
  }
}

void AssociatedWebContentsContent::OnAllPDFPagesTextReceived(
    FetchPageContentCallback callback,
    const std::vector<std::pair<size_t, std::string>>& page_texts) {
  // Pre-size vector to hold all texts in order
  std::vector<std::string> ordered_texts(page_texts.size());

  // Insert texts at their correct indices
  for (const auto& [index, text] : page_texts) {
    ordered_texts[index] = std::move(text);
  }

  std::string content;
  for (auto it = ordered_texts.begin(); it != ordered_texts.end(); ++it) {
    content.append(*it);
    if (std::next(it) != ordered_texts.end()) {
      content.append("\n");
    }
  }

  std::move(callback).Run(std::move(content), false, "");
}
#endif  // BUILDFLAG(ENABLE_PDF)

void AssociatedWebContentsContent::GetSearchSummarizerKey(
    GetSearchSummarizerKeyCallback callback) {
  if (!IsBraveSearchSERP(web_contents()->GetLastCommittedURL())) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  page_content_fetcher_delegate_->GetSearchSummarizerKey(std::move(callback));
}

void AssociatedWebContentsContent::GetOpenAIChatButtonNonce(
    mojom::PageContentExtractor::GetOpenAIChatButtonNonceCallback callback) {
  page_content_fetcher_delegate_->GetOpenAIChatButtonNonce(std::move(callback));
}

bool AssociatedWebContentsContent::HasOpenAIChatPermission() const {
  content::RenderFrameHost* rfh = web_contents()->GetPrimaryMainFrame();
  content::PermissionController* permission_controller =
      web_contents()->GetBrowserContext()->GetPermissionController();
  content::PermissionResult permission_status =
      permission_controller->GetPermissionResultForCurrentDocument(
          content::PermissionDescriptorUtil::
              CreatePermissionDescriptorForPermissionType(
                  blink::PermissionType::BRAVE_OPEN_AI_CHAT),
          rfh);
  return permission_status.status == content::PermissionStatus::GRANTED;
}

void AssociatedWebContentsContent::GetScreenshots(
    mojom::ConversationHandler::GetScreenshotsCallback callback) {
  if (print_preview_extraction_delegate_ &&
      (IsPdf(web_contents()) ||
       kPrintPreviewRetrievalHosts.contains(
           web_contents()->GetLastCommittedURL().host_piece()))) {
    // Use print preview extraction for PDFs and print preview hosts
    // when delegate is available
    print_preview_extraction_delegate_->CaptureImages(
        base::BindOnce(&AssociatedWebContentsContent::OnScreenshotsCaptured,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  } else {
    full_screenshotter_ = std::make_unique<FullScreenshotter>();
    full_screenshotter_->CaptureScreenshots(
        web_contents(),
        base::BindOnce(&AssociatedWebContentsContent::OnScreenshotsCaptured,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }
}

void AssociatedWebContentsContent::OnScreenshotsCaptured(
    mojom::ConversationHandler::GetScreenshotsCallback callback,
    base::expected<std::vector<std::vector<uint8_t>>, std::string> result) {
  if (result.has_value()) {
    std::vector<mojom::UploadedFilePtr> screenshots;
    size_t screenshot_index = 0;
    for (auto& screenshot : result.value()) {
      size_t screenshot_size = screenshot.size();
      screenshots.push_back(mojom::UploadedFile::New(
          absl::StrFormat("%s%i.png", mojom::kFullPageScreenshotPrefix,
                          screenshot_index++),
          screenshot_size, std::move(screenshot),
          mojom::UploadedFileType::kScreenshot));
    }
    std::move(callback).Run(std::move(screenshots));
  } else {
    VLOG(1) << result.error();
    std::move(callback).Run(std::nullopt);
  }
}

}  // namespace ai_chat
