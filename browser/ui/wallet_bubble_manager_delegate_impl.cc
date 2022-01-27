/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/wallet_bubble_manager_delegate_impl.h"

#include <utility>
#include <vector>

#include "base/callback.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_common_ui.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/views/frame/top_container_view.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace brave_wallet {

template <typename T>
class BraveWebUIBubbleManagerT : public WebUIBubbleManagerT<T> {
 public:
  BraveWebUIBubbleManagerT(views::View* anchor_view,
                           Browser* browser,
                           const GURL& webui_url,
                           int task_manager_string_id)
      : WebUIBubbleManagerT<T>(anchor_view,
                               browser->profile(),
                               webui_url,
                               task_manager_string_id),
        browser_(browser) {}

  base::WeakPtr<WebUIBubbleDialogView> CreateWebUIBubbleDialog() override {
    auto bubble_view = WebUIBubbleManagerT<T>::CreateWebUIBubbleDialog();
    bubble_view_ = bubble_view.get();
    web_ui_contents_for_testing_ = bubble_view_->web_view()->GetWebContents();
    // Checking if we create WalletPanelUI instance of WebUI and
    // extracting BubbleContentsWrapper class to pass real browser delegate
    // into it to redirect popups to be opened as separate windows.
    // Set a callback to be possible to activate/deactivate wallet panel from
    // typescript side
    auto contents_wrapper = WebUIBubbleManagerT<T>::cached_contents_wrapper();
    if (!contents_wrapper || !contents_wrapper->web_contents()) {
      return std::move(bubble_view);
    }
    content::WebUI* const webui = contents_wrapper->web_contents()->GetWebUI();
    if (!webui || !webui->GetController()) {
      return std::move(bubble_view);
    }
    WalletPanelUI* wallet_panel =
        webui->GetController()->template GetAs<WalletPanelUI>();
    if (!wallet_panel || !browser_ || !browser_->GetDelegateWeakPtr()) {
      return std::move(bubble_view);
    }
    // Set Browser delegate to redirect popups to be opened as Popup window
    contents_wrapper->SetWebContentsAddNewContentsDelegate(
        browser_->GetDelegateWeakPtr());
    // Pass deactivation callback for wallet panel api calls
    // The bubble disappears by default when Trezor opens a popup window
    // from the wallet panel bubble. In order to prevent it we set a callback
    // to modify panel deactivation flag from api calls in SetCloseOnDeactivate
    // inside wallet_panel_handler.cc when necessary.
    wallet_panel->SetDeactivationCallback(
        base::BindRepeating(&BraveWebUIBubbleManagerT<T>::SetCloseOnDeactivate,
                            weak_factory_.GetWeakPtr()));

    return std::move(bubble_view);
  }

  void CloseOpenedPopups() {
    auto contents_wrapper = WebUIBubbleManagerT<T>::cached_contents_wrapper();
    if (!contents_wrapper)
      return;
    for (auto tab_id : contents_wrapper->GetPopupIds()) {
      Browser* popup_browser = nullptr;
      content::WebContents* popup_contents =
          brave_wallet::GetWebContentsFromTabId(&popup_browser, tab_id);
      if (!popup_contents || !popup_browser)
        continue;
      auto delegate = popup_browser->GetDelegateWeakPtr();
      if (!delegate)
        continue;
      delegate->CloseContents(popup_contents);
    }
    contents_wrapper->GetPopupIds().clear();
  }

  const std::vector<int32_t>& GetPopupIdsForTesting() {
    auto contents_wrapper = WebUIBubbleManagerT<T>::cached_contents_wrapper();
    return contents_wrapper->GetPopupIds();
  }

  void OnWidgetDestroying(views::Widget* widget) override {
    CloseOpenedPopups();
    WebUIBubbleManagerT<T>::OnWidgetDestroying(widget);
  }

  void SetCloseOnDeactivate(bool close) {
    if (bubble_view_) {
      bubble_view_->set_close_on_deactivate(close);
    }
  }

  content::WebContents* GetWebContentsForTesting() {
    return web_ui_contents_for_testing_;
  }

 private:
  Browser* browser_ = nullptr;
  WebUIBubbleDialogView* bubble_view_ = nullptr;
  content::WebContents* web_ui_contents_for_testing_ = nullptr;
  base::WeakPtrFactory<BraveWebUIBubbleManagerT<T>> weak_factory_{this};
};

// static
std::unique_ptr<WalletBubbleManagerDelegate>
WalletBubbleManagerDelegate::Create(content::WebContents* web_contents,
                                    const GURL& webui_url) {
  return std::make_unique<WalletBubbleManagerDelegateImpl>(web_contents,
                                                           webui_url);
}

WalletBubbleManagerDelegateImpl::WalletBubbleManagerDelegateImpl(
    content::WebContents* web_contents,
    const GURL& webui_url)
    : web_contents_(web_contents), webui_url_(webui_url) {
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents_);
  DCHECK(browser);

  views::View* anchor_view;
  if (browser->is_type_normal()) {
    anchor_view = static_cast<BraveBrowserView*>(browser->window())
                      ->GetWalletButtonAnchorView();
  } else {
    anchor_view = static_cast<BrowserView*>(browser->window())->top_container();
  }

  DCHECK(anchor_view);
  webui_bubble_manager_ =
      std::make_unique<BraveWebUIBubbleManagerT<WalletPanelUI>>(
          anchor_view, browser, webui_url_, IDS_ACCNAME_BRAVE_WALLET_BUTTON);
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

void WalletBubbleManagerDelegateImpl::CloseOnDeactivate(bool close) {
  webui_bubble_manager_->SetCloseOnDeactivate(close);
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
