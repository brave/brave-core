/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "brave/components/ai_chat/content/browser/model_service_factory.h"
#include "brave/components/ai_chat/content/browser/page_content_fetcher.h"
#include "brave/components/ai_chat/content/browser/pdf_utils.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_accessibility_state.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/scoped_accessibility_mode.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "pdf/buildflags.h"
#include "ui/accessibility/ax_mode.h"
#include "ui/accessibility/ax_updates_and_events.h"
#include "ui/base/l10n/l10n_util.h"

namespace ai_chat {

namespace {
std::optional<uint32_t> g_max_page_content_length_for_testing;

}  // namespace

AIChatTabHelper::PDFA11yInfoLoadObserver::PDFA11yInfoLoadObserver(
    content::WebContents* web_contents,
    AIChatTabHelper* helper)
    : content::WebContentsObserver(web_contents), helper_(helper) {}

void AIChatTabHelper::PDFA11yInfoLoadObserver::AccessibilityEventReceived(
    const ui::AXUpdatesAndEvents& details) {
#if BUILDFLAG(ENABLE_PDF)
  for (const auto& update : details.updates) {
    for (const auto& node : update.nodes) {
      if (node.GetStringAttribute(ax::mojom::StringAttribute::kName) ==
          l10n_util::GetStringUTF8(IDS_PDF_LOADED_TO_A11Y_TREE)) {
        helper_->OnPDFA11yInfoLoaded();
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

// static
void AIChatTabHelper::SetMaxContentLengthForTesting(
    std::optional<uint32_t> max_length) {
  g_max_page_content_length_for_testing = max_length;
}

AIChatTabHelper::AIChatTabHelper(
    content::WebContents* web_contents,
    AIChatMetrics* ai_chat_metrics,
    base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
        skus_service_getter,
    PrefService* local_state_prefs,
    const std::string& channel_name)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<AIChatTabHelper>(*web_contents),
      ConversationDriver(
          user_prefs::UserPrefs::Get(web_contents->GetBrowserContext()),
          local_state_prefs,
          ModelServiceFactory::GetForBrowserContext(
              web_contents->GetBrowserContext()),
          ai_chat_metrics,
          skus_service_getter,
          web_contents->GetBrowserContext()
              ->GetDefaultStoragePartition()
              ->GetURLLoaderFactoryForBrowserProcess(),
          channel_name) {
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
    FetchPageContent(web_contents(), "",
                     std::move(pending_get_page_content_callback_));
  }
  pdf_load_observer_.reset();
  if (on_pdf_a11y_info_loaded_cb_) {
    std::move(on_pdf_a11y_info_loaded_cb_).Run();
  }
}

void AIChatTabHelper::OnPreviewTextReady(std::string ocr_text) {
  if (pending_get_page_content_callback_) {
    std::move(pending_get_page_content_callback_)
        .Run(std::move(ocr_text), false, "");
  }
}

// content::WebContentsObserver

void AIChatTabHelper::WebContentsDestroyed() {
  favicon::ContentFaviconDriver::FromWebContents(web_contents())
      ->RemoveObserver(this);
}

void AIChatTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame()) {
    return;
  }
  DVLOG(2) << __func__ << navigation_handle->GetNavigationId()
           << " url: " << navigation_handle->GetURL().spec()
           << " same document? " << navigation_handle->IsSameDocument();

  // Allow same-document navigation, as content often changes as a result
  // of framgment / pushState / replaceState navigations.
  // Content won't be retrieved immediately and we don't have a similar
  // "DOM Content Loaded" event, so let's wait for something else such as
  // page title changing before committing to starting a new conversation
  // and treating it as a "fresh page".
  is_same_document_navigation_ = navigation_handle->IsSameDocument();
  pending_navigation_id_ = navigation_handle->GetNavigationId();

  // Experimentally only call |OnNewPage| for same-page navigations _if_
  // it results in a page title change (see |TtileWasSet|).
  if (!is_same_document_navigation_) {
    OnNewPage(pending_navigation_id_);
  }
}

void AIChatTabHelper::TitleWasSet(content::NavigationEntry* entry) {
  DVLOG(3) << __func__ << entry->GetTitle();
  if (is_same_document_navigation_) {
    DVLOG(2) << "Same document navigation detected new \"page\" - calling "
                "OnNewPage()";
    // Page title modification after same-document navigation seems as good a
    // time as any to assume meaningful changes occured to the content.
    OnNewPage(pending_navigation_id_);
    // Don't respond to further TitleWasSet
    is_same_document_navigation_ = false;
  }
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
    pdf_load_observer_ =
        std::make_unique<PDFA11yInfoLoadObserver>(inner_web_contents, this);
    is_pdf_a11y_info_loaded_ = false;
  }
}

void AIChatTabHelper::OnWebContentsFocused(
    content::RenderWidgetHost* render_widget_host) {
  if (IsPdf(web_contents())) {
    CheckPDFA11yTree();
  }
}

void AIChatTabHelper::OnWebContentsLostFocus(
    content::RenderWidgetHost* render_widget_host) {
  check_pdf_a11y_tree_attempts_ = 0;
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
  if (is_same_document_navigation_) {
    DVLOG(2) << "Same document navigation detected new \"page\" - calling "
                "OnNewPage()";
    // Page title modification after same-document navigation seems as good a
    // time as any to assume meaningful changes occured to the content.
    OnNewPage(pending_navigation_id_);
    // Don't respond to further TitleWasSet
    is_same_document_navigation_ = false;
  }
}

// ai_chat::ConversationDriver

GURL AIChatTabHelper::GetPageURL() const {
  return web_contents()->GetLastCommittedURL();
}

void AIChatTabHelper::GetPageContent(GetPageContentCallback callback,
                                     std::string_view invalidation_token) {
  if (IsPdf(web_contents()) && !is_pdf_a11y_info_loaded_) {
    if (pending_get_page_content_callback_) {
      std::move(pending_get_page_content_callback_).Run("", false, "");
    }
    // invalidation_token doesn't matter for PDF extraction.
    pending_get_page_content_callback_ = std::move(callback);
  } else {
    if (base::Contains(kPrintPreviewRetrievalHosts,
                       GetPageURL().host_piece())) {
      pending_get_page_content_callback_ = std::move(callback);
      NotifyPrintPreviewRequested(false);
    } else {
      FetchPageContent(web_contents(), invalidation_token, std::move(callback));
    }
  }
}

void AIChatTabHelper::PrintPreviewFallback(GetPageContentCallback callback) {
  pending_get_page_content_callback_ = std::move(callback);
  NotifyPrintPreviewRequested(IsPdf(web_contents()));
}

std::u16string AIChatTabHelper::GetPageTitle() const {
  return web_contents()->GetTitle();
}

void AIChatTabHelper::BindPageContentExtractorReceiver(
    mojo::PendingAssociatedReceiver<mojom::PageContentExtractorHost> receiver) {
  page_content_extractor_receiver_.reset();
  page_content_extractor_receiver_.Bind(std::move(receiver));
}

uint32_t AIChatTabHelper::GetMaxPageContentLength() {
  if (g_max_page_content_length_for_testing) {
    return *g_max_page_content_length_for_testing;
  }
  auto& model = GetCurrentModel();
  if (model.options->is_leo_model_options()) {
    return model.options->get_leo_model_options()->max_page_content_length;
  } else {
    return kCustomModelMaxPageContentLength;
  }
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
      // Reset the counter for next OnWebContentsFocused
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

WEB_CONTENTS_USER_DATA_KEY_IMPL(AIChatTabHelper);

}  // namespace ai_chat
