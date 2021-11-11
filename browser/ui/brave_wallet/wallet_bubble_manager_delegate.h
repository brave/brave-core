/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_WALLET_WALLET_BUBBLE_MANAGER_DELEGATE_H_
#define BRAVE_BROWSER_UI_BRAVE_WALLET_WALLET_BUBBLE_MANAGER_DELEGATE_H_

#include <memory>
#include <vector>

class GURL;

namespace content {
class WebContents;
}  // namespace content

namespace brave_wallet {

class WalletBubbleManagerDelegate {
 public:
  static std::unique_ptr<WalletBubbleManagerDelegate> Create(
      content::WebContents* web_contents,
      const GURL& webui_url);

  WalletBubbleManagerDelegate(const WalletBubbleManagerDelegate&) = delete;
  WalletBubbleManagerDelegate& operator=(const WalletBubbleManagerDelegate&) =
      delete;

  virtual ~WalletBubbleManagerDelegate() = default;

  virtual void ShowBubble() = 0;
  virtual void CloseBubble() = 0;
  virtual bool IsShowingBubble() = 0;
  virtual bool IsBubbleClosedForTesting() = 0;
  virtual content::WebContents* GetWebContentsForTesting() = 0;
  virtual const std::vector<int32_t>& GetPopupIdsForTesting() = 0;
  virtual void CloseOnDeactivate(bool close) = 0;

 protected:
  WalletBubbleManagerDelegate() = default;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_UI_BRAVE_WALLET_WALLET_BUBBLE_MANAGER_DELEGATE_H_
