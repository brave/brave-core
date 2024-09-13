/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "base/containers/fixed_flat_set.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "brave/components/ai_chat/content/browser/page_content_fetcher.h"
#include "brave/components/ai_chat/content/browser/pdf_utils.h"
#include "brave/components/ai_chat/core/browser/associated_content_driver.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/browser_accessibility_state.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/scoped_accessibility_mode.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "pdf/buildflags.h"
#include "ui/accessibility/ax_mode.h"
#include "ui/accessibility/ax_updates_and_events.h"
#include "ui/base/l10n/l10n_util.h"

namespace ai_chat {


AIChatTabHelper::PDFA11yInfoLoadObserver::PDFA11yInfoLoadObserver(
    content::WebContents* web_contents,
    AIChatTabHelper* helper)
    : content::WebContentsObserver(web_contents), helper_(helper) {}

void AIChatTabHelper::PDFA11yInfoLoadObserver::AccessibilityEventReceived(
    const ui::AXUpdatesAndEvents& details) {
#if BUILDFLAG(ENABLE_PDF)
  for (const auto& update : details.updates) {
    for (const auto& node : update.nodes) {
      const auto& node_name =
          node.GetStringAttribute(ax::mojom::StringAttribute::kName);
      if (node_name == l10n_util::GetStringUTF8(IDS_PDF_LOADED_TO_A11Y_TREE) ||
          node_name == l10n_util::GetStringUTF8(IDS_PDF_OCR_COMPLETED) ||
          node_name == l10n_util::GetStringUTF8(IDS_PDF_OCR_NO_RESULT)) {
        // features::kUseMoveNotCopyInMergeTreeUpdate updates a11y tree after
        // `AccessibilityEventReceived` so we cannot assume changes are
        // reflected upon receiving updates.
        helper_->CheckPDFA11yTree();
        break;
      }
    }
  }
#endif
}

AIChatTabHelper::PDFA11yInfoLoadObserver::~PDFA11yInfoLoadObserver() = default;

// static
void AIChatTabHelper::BindPageContentExtractorHost(
    content::RenderFrameHost* rfh,
    mojo::PendingAssociatedReceiver<mojom::PageContentExtractorHost> receiver) {
  CHECK(rfh);
  if (!rfh->IsInPrimaryMainFrame()) {
    DVLOG(4) << "Not binding extractor host to non-main frame";
    return;
  }
  auto* sender = content::WebContents::FromRenderFrameHost(rfh);
  if (!sender) {
    DVLOG(1) << "Cannot bind extractor host, no valid WebContents";
    return;
  }
  auto* tab_helper = AIChatTabHelper::FromWebContents(sender);
  if (!tab_helper) {
    DVLOG(1) << "Cannot bind extractor host, no AIChatTabHelper - "
             << sender->GetVisibleURL();
    return;
  }
  DVLOG(4) << "Binding extractor host to AIChatTabHelper";
  tab_helper->BindPageContentExtractorReceiver(std::move(receiver));
}

AIChatTabHelper::AIChatTabHelper(content::WebContents* web_contents,
                                 std::unique_ptr<PrintPreviewExtractionDelegate>
                                     print_preview_extraction_delegate)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<AIChatTabHelper>(*web_contents),
      AssociatedContentDriver(web_contents->GetBrowserContext()
                                  ->GetDefaultStoragePartition()
                                  ->GetURLLoaderFactoryForBrowserProcess()),
      print_preview_extraction_delegate_(
          std::move(print_preview_extraction_delegate)),
      page_content_fetcher_delegate_(
          std::make_unique<PageContentFetcher>(web_contents)) {
  favicon::ContentFaviconDriver::FromWebContents(web_contents)
      ->AddObserver(this);
}

AIChatTabHelper::~AIChatTabHelper() = default;

void AIChatTabHelper::SetOnPDFA11yInfoLoadedCallbackForTesting(
    base::OnceClosure cb) {
  on_pdf_a11y_info_loaded_cb_ = std::move(cb);
}

void AIChatTabHelper::OnPDFA11yInfoLoaded() {
  DVLOG(3) << " PDF Loaded: " << GetPageURL();
  is_pdf_a11y_info_loaded_ = true;
  if (pending_get_page_content_callback_) {
    // Call GetPageContent again so that we can still utilize fallback
    // to print preview for PDF if needed.
    GetPageContent(std::move(pending_get_page_content_callback_), "");
  }
  pdf_load_observer_.reset();
  if (on_pdf_a11y_info_loaded_cb_) {
    std::move(on_pdf_a11y_info_loaded_cb_).Run();
  }
}

// content::WebContentsObserver

void AIChatTabHelper::WebContentsDestroyed() {
  favicon::ContentFaviconDriver::FromWebContents(web_contents())
      ->RemoveObserver(this);
  inner_web_contents_ = nullptr;
}

void AIChatTabHelper::NavigationEntryCommitted(
    const content::LoadCommittedDetails& load_details) {
  if (!load_details.is_main_frame) {
    return;
  }
  // UniqueID will provide a consitent value for the entry when navigating
  // through history, allowing us to re-join conversations and navigations.
  int pending_navigation_id = load_details.entry->GetUniqueID();
  pending_navigation_id_ = pending_navigation_id;
  DVLOG(2) << __func__ << " id: " << pending_navigation_id_
           << " url: " << load_details.entry->GetVirtualURL()
           << " title: " << load_details.entry->GetTitle() << " same document? "
           << load_details.is_same_document;
  // Allow same-document navigation, as content often changes as a result
  // of framgment / pushState / replaceState navigations.
  // Content won't be retrieved immediately and we don't have a similar
  // "DOM Content Loaded" event, so let's wait for something else such as
  // page title changing before committing to starting a new conversation
  // and treating it as a "fresh page".
  is_same_document_navigation_ = load_details.is_same_document;
  // Experimentally only call |OnNewPage| for same-page navigations _if_
  // it results in a page title change (see |TtileWasSet|).
  if (!is_same_document_navigation_) {
    OnNewPage(pending_navigation_id_);
  }
}

void AIChatTabHelper::TitleWasSet(content::NavigationEntry* entry) {
  DVLOG(3) << __func__ << entry->GetTitle();
  MaybeSameDocumentIsNewPage();
}

void AIChatTabHelper::InnerWebContentsAttached(
    content::WebContents* inner_web_contents,
    content::RenderFrameHost* render_frame_host,
    bool is_full_page) {
  // Setting a11y mode for PDF process which is dedicated for each
  // PDF so we don't have to unset it.
  if (IsPdf(content::WebContents::FromRenderFrameHost(render_frame_host))) {
    // We need `AXMode::kNativeAPIs` for accessing pdf a11y info and
    // `AXMode::kWebContents` for observing a11y events from WebContents.
    bool mode_change_needed = false;
    auto current_mode = inner_web_contents->GetAccessibilityMode();
    if (!current_mode.has_mode(ui::AXMode::kNativeAPIs)) {
      current_mode |= ui::AXMode::kNativeAPIs;
      mode_change_needed = true;
    }
    if (!current_mode.has_mode(ui::AXMode::kWebContents)) {
      current_mode |= ui::AXMode::kWebContents;
      mode_change_needed = true;
    }
    if (mode_change_needed) {
      scoped_accessibility_mode_ =
          content::BrowserAccessibilityState::GetInstance()
              ->CreateScopedModeForWebContents(inner_web_contents,
                                               current_mode);
    }
    inner_web_contents_ = inner_web_contents;
    pdf_load_observer_ =
        std::make_unique<PDFA11yInfoLoadObserver>(inner_web_contents, this);
    is_pdf_a11y_info_loaded_ = false;
  }
}

// favicon::FaviconDriverObserver
void AIChatTabHelper::OnFaviconUpdated(
    favicon::FaviconDriver* favicon_driver,
    NotificationIconType notification_icon_type,
    const GURL& icon_url,
    bool icon_url_changed,
    const gfx::Image& image) {
  OnFaviconImageDataChanged();
}

// mojom::PageContentExtractorHost
void AIChatTabHelper::OnInterceptedPageContentChanged() {
  // Maybe mark that the page changed, if we didn't detect it already via title
  // change after a same-page navigation. This is the main benefit of this
  // function.
  MaybeSameDocumentIsNewPage();
}

GURL AIChatTabHelper::GetPageURL() const {
  return web_contents()->GetLastCommittedURL();
}

void AIChatTabHelper::GetPageContent(
    ConversationHandler::GetPageContentCallback callback,
    std::string_view invalidation_token) {
  bool is_pdf = IsPdf(web_contents());
  if (is_pdf && !is_pdf_a11y_info_loaded_) {
    if (pending_get_page_content_callback_) {
      // TODO(petemill): Queue the callback in a Signal, instead of only
      // allowing a single pending callback.
      std::move(pending_get_page_content_callback_).Run("", false, "");
    }
    // invalidation_token doesn't matter for PDF extraction.
    pending_get_page_content_callback_ = std::move(callback);
    // PdfAccessibilityTree::AccessibilityModeChanged handles kPDFOcr changes
    // with |always_load_or_reload_accessibility| is true
    if (inner_web_contents_) {
      auto current_mode = inner_web_contents_->GetAccessibilityMode();
      if (!current_mode.has_mode(ui::AXMode::kPDFOcr)) {
        current_mode |= ui::AXMode::kPDFOcr;
        scoped_accessibility_mode_ =
            content::BrowserAccessibilityState::GetInstance()
                ->CreateScopedModeForWebContents(inner_web_contents_,
                                                 current_mode);
      }
      pdf_load_observer_ =
          std::make_unique<PDFA11yInfoLoadObserver>(inner_web_contents_, this);
    }
    // Manually check when pdf extraction requested so we don't always rely on
    // a11y events to prevent stale callback. It can happens during background
    // pdf tab loading or bug in upstream kPdfOCR that an empty page in pdf will
    // cause PdfOcrHelper::AreAllPagesOcred() return false becasue it won't get
    // Ocred but `remaining_page_count_` was set to total pdf page count. Hence
    // status IDS_PDF_OCR_COMPLETED or IDS_PDF_OCR_NO_RESULT won't be set.
    CheckPDFA11yTree();
    return;
  }
  if (base::Contains(kPrintPreviewRetrievalHosts, GetPageURL().host_piece()) &&
      print_preview_extraction_delegate_ != nullptr) {
    // Get content using a printing / OCR mechanism, instead of
    // directly from the source.
    print_preview_extraction_delegate_->Extract(
        is_pdf,
        base::BindOnce(&AIChatTabHelper::OnExtractPrintPreviewContentComplete,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  } else {
    page_content_fetcher_delegate_->FetchPageContent(
        invalidation_token,
        base::BindOnce(&AIChatTabHelper::OnFetchPageContentComplete,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }
}

void AIChatTabHelper::OnFetchPageContentComplete(
    ConversationHandler::GetPageContentCallback callback,
    std::string content,
    bool is_video,
    std::string invalidation_token) {
  base::TrimWhitespaceASCII(content, base::TRIM_ALL, &content);
  if (content.empty() && !is_video &&
      print_preview_extraction_delegate_ != nullptr) {
    DVLOG(1) << "Initiating print preview fallback";
    print_preview_extraction_delegate_->Extract(
        IsPdf(web_contents()),
        base::BindOnce(&AIChatTabHelper::OnExtractPrintPreviewContentComplete,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
    return;
  }
  std::move(callback).Run(std::move(content), is_video,
                          std::move(invalidation_token));
}

void AIChatTabHelper::OnExtractPrintPreviewContentComplete(
    ConversationHandler::GetPageContentCallback callback,
    std::string content) {
  // Invalidation token not applicable for print preview OCR
  std::move(callback).Run(std::move(content), false, "");
}

std::u16string AIChatTabHelper::GetPageTitle() const {
  return web_contents()->GetTitle();
}

void AIChatTabHelper::MaybeSameDocumentIsNewPage() {
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

void AIChatTabHelper::BindPageContentExtractorReceiver(
    mojo::PendingAssociatedReceiver<mojom::PageContentExtractorHost> receiver) {
  page_content_extractor_receiver_.reset();
  page_content_extractor_receiver_.Bind(std::move(receiver));
}

void AIChatTabHelper::CheckPDFA11yTree() {
  DVLOG(3) << __func__ << ": " << GetPageURL();
  auto* primary_rfh = web_contents()->GetPrimaryMainFrame();
  if (!primary_rfh || is_pdf_a11y_info_loaded_) {
    return;
  }

  auto* pdf_root = GetPdfRoot(primary_rfh);
  if (!IsPdfLoaded(pdf_root)) {
    DVLOG(4) << "Waiting for PDF a11y tree ready, scheduled next check.";
    if (++check_pdf_a11y_tree_attempts_ >= 5) {
      DVLOG(4) << "PDF a11y tree not ready after 5 attempts.";
      // Reset the counter for next check
      check_pdf_a11y_tree_attempts_ = 0;
      return;
    }
    base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&AIChatTabHelper::CheckPDFA11yTree,
                       weak_ptr_factory_.GetWeakPtr()),
        base::Seconds(3));
    return;
  }
  OnPDFA11yInfoLoaded();
}

void AIChatTabHelper::GetSearchSummarizerKey(
    mojom::PageContentExtractor::GetSearchSummarizerKeyCallback callback) {
  if (!IsBraveSearchSERP(GetPageURL())) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  page_content_fetcher_delegate_->GetSearchSummarizerKey(std::move(callback));
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(AIChatTabHelper);

}  // namespace ai_chat
