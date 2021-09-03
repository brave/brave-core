/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_TAB_HELPER_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_TAB_HELPER_H_

#include <memory>
#include <utility>

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
  void ShowApproveWalletBubble();
  void CloseBubble();
  bool IsShowingBubble();
  bool IsBubbleClosedForTesting();
  void SetShowBubbleCallbackForTesting(base::OnceClosure callback) {
    show_bubble_callback_for_testing_ = std::move(callback);
  }
#endif

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveWalletTabHelperUnitTest, GetApproveBubbleURL);
  friend class content::WebContentsUserData<BraveWalletTabHelper>;
#if !defined(OS_ANDROID) && !defined(OS_IOS)
  GURL GetBubbleURL();
  base::OnceClosure show_bubble_callback_for_testing_;
  GURL GetApproveBubbleURL();
  std::unique_ptr<WalletBubbleManagerDelegate> wallet_bubble_manager_delegate_;
#endif
  content::WebContents* web_contents_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_TAB_HELPER_H_
