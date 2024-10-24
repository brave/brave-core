/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"

#include <array>
#include <cstdint>
#include <functional>
#include <ios>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/numerics/clamped_math.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_ostream_operators.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/content/browser/full_screenshotter.h"
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
#include "content/public/browser/permission_controller.h"
#include "content/public/browser/permission_result.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/scoped_accessibility_mode.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "pdf/buildflags.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"
#include "third_party/blink/public/mojom/permissions/permission_status.mojom-shared.h"
#include "ui/accessibility/ax_enums.mojom-shared.h"
#include "ui/accessibility/ax_mode.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/accessibility/ax_tree_update.h"
#include "ui/accessibility/ax_updates_and_events.h"
#include "ui/base/l10n/l10n_util.h"

namespace favicon {
class FaviconDriver;
}  // namespace favicon
namespace gfx {
class Image;
}  // namespace gfx

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
      if (node_name == l10n_util::GetStringUTF8(IDS_PDF_LOADED_TO_A11Y_TREE)) {
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
  previous_page_title_ = web_contents->GetTitle();
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
  // UniqueID will provide a consistent value for the entry when navigating
  // through history, allowing us to re-join conversations and navigations.
  int pending_navigation_id = load_details.entry->GetUniqueID();
  pending_navigation_id_ = pending_navigation_id;
  DVLOG(2) << __func__ << " id: " << pending_navigation_id_
           << "\n url: " << load_details.entry->GetVirtualURL()
           << "\n current page title: " << GetPageTitle()
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
  if (!is_same_document_navigation_ || previous_page_title_ != GetPageTitle()) {
    OnNewPage(pending_navigation_id_);
  }
  previous_page_title_ = GetPageTitle();
}

void AIChatTabHelper::TitleWasSet(content::NavigationEntry* entry) {
  DVLOG(2) << __func__ << ": id=" << entry->GetUniqueID()
           << " title=" << entry->GetTitle();
  MaybeSameDocumentIsNewPage();
  previous_page_title_ = GetPageTitle();
  OnTitleChanged();
}

void AIChatTabHelper::InnerWebContentsAttached(
    content::WebContents* inner_web_contents,
    content::RenderFrameHost* render_frame_host) {
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

void AIChatTabHelper::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                                    const GURL& validated_url) {
  DVLOG(4) << __func__ << ": " << validated_url.spec();
  if (validated_url == GetPageURL()) {
    is_page_loaded_ = true;
    if (pending_get_page_content_callback_) {
      GetPageContent(std::move(pending_get_page_content_callback_), "");
    }
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

void AIChatTabHelper::GetPageContent(GetPageContentCallback callback,
                                     std::string_view invalidation_token) {
  bool is_pdf = IsPdf(web_contents());
  if (is_pdf && !is_pdf_a11y_info_loaded_) {
    SetPendingGetContentCallback(std::move(callback));
    // Manually check when pdf extraction requested so we don't always rely on
    // a11y events to prevent stale callback. It can happens during background
    // pdf tab loading or bug in upstream kPdfOCR that an empty page in pdf will
    // cause PdfOcrHelper::AreAllPagesOcred() return false becasue it won't get
    // Ocred but `remaining_page_count_` was set to total pdf page count. Hence
    // status IDS_PDF_OCR_COMPLETED or IDS_PDF_OCR_NO_RESULT won't be set.
    CheckPDFA11yTree();
    return;
  }
  if (kPrintPreviewRetrievalHosts.contains(GetPageURL().host_piece())) {
    // Get content using a printing / OCR mechanism, instead of
    // directly from the source, if available.
    DVLOG(1) << __func__ << " print preview url";
    if (MaybePrintPreviewExtract(callback)) {
      return;
    }
  }

  page_content_fetcher_delegate_->FetchPageContent(
      invalidation_token,
      base::BindOnce(&AIChatTabHelper::OnFetchPageContentComplete,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AIChatTabHelper::OnFetchPageContentComplete(
    GetPageContentCallback callback,
    std::string content,
    bool is_video,
    std::string invalidation_token) {
  base::TrimWhitespaceASCII(content, base::TRIM_ALL, &content);
  // If content is empty, and page was not loaded yet, wait for page load.
  // Once page load is complete, try again. If it's still empty, fallback
  // to print preview extraction.
  if (content.empty() && !is_video) {
    // When page isn't loaded yet, wait until DidFinishLoad
    DVLOG(1) << __func__ << " empty content, will attempt fallback";
    if (MaybePrintPreviewExtract(callback)) {
      return;
    } else if (!is_page_loaded_) {
      DVLOG(1) << "page was not loaded yet, will try again after load";
      SetPendingGetContentCallback(std::move(callback));
      return;
    }
    // When print preview extraction isn't available, return empty content
    DVLOG(1) << "no fallback available";
  }

  full_screenshotter_ = std::make_unique<FullScreenshotter>();
  full_screenshotter_->CaptureScreenshot(
      web_contents(),
      base::BindOnce(&AIChatTabHelper::OnCaptureScreenshotComplete,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     content, is_video, invalidation_token));
}

void AIChatTabHelper::OnCaptureScreenshotComplete(
    GetPageContentCallback callback,
    std::string content,
    bool is_video,
    std::string invalidation_token,
    base::expected<std::vector<std::string>, std::string> result) {
  if (result.has_value()) {
    LOG(ERROR) << "screenshots number: " << result->size();
    std::move(callback).Run(std::move(content), is_video,
                            std::move(invalidation_token),
                            std::move(result.value()));
  } else {
    LOG(ERROR) << result.error();
    VLOG(1) << result.error();
    std::move(callback).Run(std::move(content), is_video,
                            std::move(invalidation_token), {});
  }
}

void AIChatTabHelper::SetPendingGetContentCallback(
    GetPageContentCallback callback) {
  if (pending_get_page_content_callback_) {
    std::move(pending_get_page_content_callback_).Run("", false, "", {});
  }
  pending_get_page_content_callback_ = std::move(callback);
}

bool AIChatTabHelper::MaybePrintPreviewExtract(
    GetPageContentCallback& callback) {
  if (print_preview_extraction_delegate_ == nullptr) {
    DVLOG(1) << "print preview extraction not supported";
    return false;
  }
  if (!is_page_loaded_) {
    DVLOG(1) << "will extract print preview content when page is loaded";
    SetPendingGetContentCallback(std::move(callback));
  } else {
    // When page is already loaded, fallback to print preview extraction
    DVLOG(1) << "extracting print preview content now";
    print_preview_extraction_delegate_->Extract(
        IsPdf(web_contents()),
        base::BindOnce(&AIChatTabHelper::OnExtractPrintPreviewContentComplete,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }
  return true;
}

void AIChatTabHelper::OnExtractPrintPreviewContentComplete(
    GetPageContentCallback callback,
    std::string content) {
  // Invalidation token not applicable for print preview OCR
  std::move(callback).Run(std::move(content), false, "", {});
}

std::u16string AIChatTabHelper::GetPageTitle() const {
  return web_contents()->GetTitle();
}

void AIChatTabHelper::OnNewPage(int64_t navigation_id) {
  DVLOG(3) << __func__ << " id: " << navigation_id;
  AssociatedContentDriver::OnNewPage(navigation_id);
  if (pending_get_page_content_callback_) {
    std::move(pending_get_page_content_callback_).Run("", false, "", {});
  }
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
    GetSearchSummarizerKeyCallback callback) {
  if (!IsBraveSearchSERP(GetPageURL())) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  page_content_fetcher_delegate_->GetSearchSummarizerKey(std::move(callback));
}

void AIChatTabHelper::GetOpenAIChatButtonNonce(
    mojom::PageContentExtractor::GetOpenAIChatButtonNonceCallback callback) {
  page_content_fetcher_delegate_->GetOpenAIChatButtonNonce(std::move(callback));
}

bool AIChatTabHelper::HasOpenAIChatPermission() const {
  content::RenderFrameHost* rfh = web_contents()->GetPrimaryMainFrame();
  content::PermissionController* permission_controller =
      web_contents()->GetBrowserContext()->GetPermissionController();
  content::PermissionResult permission_status =
      permission_controller->GetPermissionResultForCurrentDocument(
          blink::PermissionType::BRAVE_OPEN_AI_CHAT, rfh);
  return permission_status.status == content::PermissionStatus::GRANTED;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(AIChatTabHelper);

}  // namespace ai_chat
