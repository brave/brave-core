/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_TAB_HELPER_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_TAB_HELPER_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "brave/components/brave_wallet/browser/brave_wallet_types.h"
#include "content/public/browser/web_contents_user_data.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
class WebContents;

}  // namespace content

namespace brave_wallet {

class WalletBubbleManagerDelegate;

class BraveWalletTabHelper
    : public content::WebContentsUserData<BraveWalletTabHelper> {
 public:
  explicit BraveWalletTabHelper(content::WebContents* web_contents);
  ~BraveWalletTabHelper() override;

#if !defined(OS_ANDROID) && !defined(OS_IOS)
  void ShowBubble();
  void RequestUserApproval(const std::string& requestData,
                           RequestEthereumChainCallback callback);
  void UserRequestCompleted(const std::string& requestData,
                            const std::string& result);
  void CloseBubble();
  bool IsShowingBubble();
  bool IsBubbleClosedForTesting();
#endif

 private:
  friend class content::WebContentsUserData<BraveWalletTabHelper>;
#if !defined(OS_ANDROID) && !defined(OS_IOS)
  GURL GetBubbleURL();
  std::unordered_map<size_t, RequestEthereumChainCallback> request_callbacks_;
  std::unique_ptr<WalletBubbleManagerDelegate> wallet_bubble_manager_delegate_;
#endif
  content::WebContents* web_contents_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_TAB_HELPER_H_
