/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/wallet_bubble_manager_delegate_impl.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_common_ui.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_panel_ui.h"
#include "chrome/browser/file_select_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "chrome/browser/ui/views/frame/top_container_view.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace brave_wallet {

class WalletWebUIBubbleDialogView : public WebUIBubbleDialogView {
  METADATA_HEADER(WalletWebUIBubbleDialogView, WebUIBubbleDialogView)
 public:
  WalletWebUIBubbleDialogView(
      views::View* anchor_view,
      WebUIContentsWrapper* contents_wrapper,
      const std::optional<gfx::Rect>& anchor_rect = std::nullopt,
      views::BubbleBorder::Arrow arrow = views::BubbleBorder::TOP_RIGHT)
      : WebUIBubbleDialogView(anchor_view,
                              contents_wrapper->GetWeakPtr(),
                              anchor_rect,
                              arrow) {
    set_close_on_deactivate(false);

    CHECK(anchor_widget());
    anchor_widget_observation_.Observe(
        anchor_widget()->GetPrimaryWindowWidget());
  }

  WalletWebUIBubbleDialogView(const WalletWebUIBubbleDialogView&) = delete;
  WalletWebUIBubbleDialogView& operator=(const WalletWebUIBubbleDialogView&) =
      delete;
  ~WalletWebUIBubbleDialogView() override = default;

  void RunFileChooser(content::RenderFrameHost* render_frame_host,
                      scoped_refptr<content::FileSelectListener> listener,
                      const blink::mojom::FileChooserParams& params) override {
    FileSelectHelper::RunFileChooser(render_frame_host, std::move(listener),
                                     params);
  }

 private:
  // WidgetObserver:
  void OnWidgetTreeActivated(views::Widget* root_widget,
                             views::Widget* active_widget) override {
    // Similar to what `ExtensionPopup::OnWidgetTreeActivated` does.

    if (!GetWidget()->IsVisible()) {
      return;
    }

    if (active_widget != GetWidget()) {
      GetWidget()->CloseWithReason(views::Widget::ClosedReason::kLostFocus);
    }
  }

  base::ScopedObservation<views::Widget, views::WidgetObserver>
      anchor_widget_observation_{this};
};

BEGIN_METADATA(WalletWebUIBubbleDialogView)
END_METADATA

class WalletWebUIBubbleManager : public WebUIBubbleManagerImpl<WalletPanelUI>,
                                 public views::ViewObserver {
 public:
  WalletWebUIBubbleManager(views::View* anchor_view,
                           Browser* browser,
                           const GURL& webui_url,
                           int task_manager_string_id,
                           bool force_load_on_create)
      : WebUIBubbleManagerImpl(anchor_view,
                               browser,
                               webui_url,
                               task_manager_string_id,
                               force_load_on_create),
        browser_(browser),
        anchor_view_(anchor_view) {}

  base::WeakPtr<WebUIBubbleDialogView> CreateWebUIBubbleDialog(
      const std::optional<gfx::Rect>& anchor,
      views::BubbleBorder::Arrow arrow) override {
    // This is prevent duplicate logic of cached_contents_wrapper creation
    // so we close WebUIBubbleDialogView and re-create bubble with
    // WalletWebUIBubbleDialogView.
    auto bubble_view_to_close =
        WebUIBubbleManagerImpl::CreateWebUIBubbleDialog(anchor, arrow);
    auto* widget_to_close = bubble_view_to_close->GetWidget();
    if (widget_to_close) {
      widget_to_close->CloseNow();
    }
    auto* contents_wrapper = cached_contents_wrapper();
    CHECK(contents_wrapper);
    auto bubble_view = std::make_unique<WalletWebUIBubbleDialogView>(
        anchor_view_, contents_wrapper, anchor, arrow);
    bubble_view_ = bubble_view->GetWeakPtr();
    auto* widget =
        views::BubbleDialogDelegateView::CreateBubble(std::move(bubble_view));
    CHECK(widget);
    widget->SetZOrderLevel(ui::ZOrderLevel::kSecuritySurface);

    // Checking if we create WalletPanelUI instance of WebUI and
    // extracting WebUIContentsWrapper class to pass real browser delegate
    // into it to redirect popups to be opened as separate windows.
    // Set a callback to be possible to activate/deactivate wallet panel from
    // typescript side
    if (!contents_wrapper->web_contents()) {
      return bubble_view_;
    }
    content::WebUI* const webui = contents_wrapper->web_contents()->GetWebUI();
    if (!webui || !webui->GetController()) {
      return bubble_view_;
    }
    WalletPanelUI* wallet_panel =
        webui->GetController()->template GetAs<WalletPanelUI>();
    if (!wallet_panel || !browser_ || !browser_->AsWeakPtr()) {
      return bubble_view_;
    }
    // Set Browser delegate to redirect popups to be opened as Popup window
    contents_wrapper->SetWebContentsAddNewContentsDelegate(
        browser_->AsWeakPtr());

    return bubble_view_;
  }

  void CloseOpenedPopups() {
    auto* contents_wrapper = cached_contents_wrapper();
    if (!contents_wrapper) {
      return;
    }
    for (auto tab_id : contents_wrapper->popup_ids()) {
      Browser* popup_browser = nullptr;
      content::WebContents* popup_contents =
          brave_wallet::GetWebContentsFromTabId(&popup_browser, tab_id);
      if (!popup_contents || !popup_browser) {
        continue;
      }
      base::WeakPtr<content::WebContentsDelegate> delegate =
          popup_browser->AsWeakPtr();
      if (!delegate) {
        continue;
      }
      delegate->CloseContents(popup_contents);
    }
    contents_wrapper->ClearPopupIds();
  }

  const std::vector<int32_t>& GetPopupIdsForTesting() {
    auto* contents_wrapper = cached_contents_wrapper();
    return contents_wrapper->popup_ids();
  }

  void OnWidgetDestroying(views::Widget* widget) override {
    CloseOpenedPopups();
    WebUIBubbleManagerImpl::OnWidgetDestroying(widget);
  }

  content::WebContents* GetWebContentsForTesting() {
    if (!bubble_view_) {
      return nullptr;
    }
    return bubble_view_->web_view()->GetWebContents();
  }

 private:
  const raw_ptr<Browser> browser_;
  const raw_ptr<views::View> anchor_view_;
  base::WeakPtr<WebUIBubbleDialogView> bubble_view_ = nullptr;
  base::WeakPtrFactory<WalletWebUIBubbleManager> weak_factory_{this};
};

// static
std::unique_ptr<WalletBubbleManagerDelegate>
WalletBubbleManagerDelegate::MaybeCreate(content::WebContents* web_contents,
                                         const GURL& webui_url) {
  if (!IsAllowedForContext(web_contents->GetBrowserContext())) {
    return nullptr;
  }

  return std::make_unique<WalletBubbleManagerDelegateImpl>(web_contents,
                                                           webui_url);
}

WalletBubbleManagerDelegateImpl::WalletBubbleManagerDelegateImpl(
    content::WebContents* web_contents,
    const GURL& webui_url)
    : web_contents_(web_contents), webui_url_(webui_url) {
  Browser* browser = chrome::FindBrowserWithTab(web_contents_);
  DCHECK(browser);

  views::View* anchor_view;
  if (browser->is_type_normal()) {
    anchor_view = static_cast<BraveBrowserView*>(browser->window())
                      ->GetWalletButtonAnchorView();
  } else {
    anchor_view = static_cast<BrowserView*>(browser->window())->top_container();
  }

  DCHECK(anchor_view);
  webui_bubble_manager_ = std::make_unique<WalletWebUIBubbleManager>(
      anchor_view, browser, webui_url_, IDS_ACCNAME_BRAVE_WALLET_BUTTON,
      /*force_load_on_create=*/false);
}

WalletBubbleManagerDelegateImpl::~WalletBubbleManagerDelegateImpl() {
  webui_bubble_manager_->CloseBubble();
}

void WalletBubbleManagerDelegateImpl::ShowBubble() {
  // Suppress request if not from active web_contents.
  if (GetActiveWebContents() != web_contents_) {
    return;
  }

  webui_bubble_manager_->ShowBubble();
}

content::WebContents*
WalletBubbleManagerDelegateImpl::GetWebContentsForTesting() {
  return webui_bubble_manager_->GetWebContentsForTesting();
}

const std::vector<int32_t>&
WalletBubbleManagerDelegateImpl::GetPopupIdsForTesting() {
  return webui_bubble_manager_->GetPopupIdsForTesting();
}
void WalletBubbleManagerDelegateImpl::CloseBubble() {
  webui_bubble_manager_->CloseBubble();
}

bool WalletBubbleManagerDelegateImpl::IsShowingBubble() {
  return webui_bubble_manager_ && webui_bubble_manager_->GetBubbleWidget();
}

bool WalletBubbleManagerDelegateImpl::IsBubbleClosedForTesting() {
  return !webui_bubble_manager_ || !webui_bubble_manager_->GetBubbleWidget() ||
         webui_bubble_manager_->GetBubbleWidget()->IsClosed();
}

}  // namespace brave_wallet
