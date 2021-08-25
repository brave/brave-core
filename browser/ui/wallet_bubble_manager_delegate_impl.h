/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WALLET_BUBBLE_MANAGER_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_UI_WALLET_BUBBLE_MANAGER_DELEGATE_IMPL_H_

#include "brave/browser/ui/brave_wallet/wallet_bubble_manager_delegate.h"

#include <memory>

#include "brave/browser/ui/webui/brave_wallet/wallet_panel_ui.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "url/gurl.h"

namespace brave_wallet {

class WalletBubbleManagerDelegateImpl : public WalletBubbleManagerDelegate {
 public:
  explicit WalletBubbleManagerDelegateImpl(content::WebContents* web_contents,
                                           const GURL& webui_url);
  WalletBubbleManagerDelegateImpl(const WalletBubbleManagerDelegateImpl&) =
      delete;
  WalletBubbleManagerDelegateImpl& operator=(
      const WalletBubbleManagerDelegateImpl&) = delete;
  ~WalletBubbleManagerDelegateImpl() override;

  void ShowBubble() override;
  void CloseBubble() override;
  bool IsShowingBubble() override;
  bool IsBubbleClosedForTesting() override;

 private:
  content::WebContents* web_contents_;
  GURL webui_url_;
  std::unique_ptr<WebUIBubbleManagerT<WalletPanelUI>> webui_bubble_manager_;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_UI_WALLET_BUBBLE_MANAGER_DELEGATE_IMPL_H_
