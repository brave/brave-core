/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/wallet_bubble_manager_delegate_impl.h"

#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace {

content::WebContents* GetActiveWebContents() {
  return BrowserList::GetInstance()
      ->GetLastActive()
      ->tab_strip_model()
      ->GetActiveWebContents();
}

}  // namespace

namespace brave_wallet {

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

  views::View* anchor_view = static_cast<BraveBrowserView*>(browser->window())
                                 ->GetWalletButtonAnchorView();
  DCHECK(anchor_view);

  webui_bubble_manager_ = std::make_unique<WebUIBubbleManagerT<WalletPanelUI>>(
      anchor_view, browser->profile(), webui_url_,
      IDS_ACCNAME_BRAVE_WALLET_BUTTON, true);
}

WalletBubbleManagerDelegateImpl::~WalletBubbleManagerDelegateImpl() = default;

void WalletBubbleManagerDelegateImpl::ShowBubble() {
  // Suppress request if not from active web_contents.
  if (GetActiveWebContents() != web_contents_) {
    return;
  }

  webui_bubble_manager_->ShowBubble();
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
