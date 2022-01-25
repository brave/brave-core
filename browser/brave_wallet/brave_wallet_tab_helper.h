/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_TAB_HELPER_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_TAB_HELPER_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/memory/raw_ptr.h"
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
  void SetCloseOnDeactivate(bool close);
  bool IsBubbleClosedForTesting();
  content::WebContents* GetBubbleWebContentsForTesting();
  const std::vector<int32_t>& GetPopupIdsForTesting();
  void SetShowBubbleCallbackForTesting(base::OnceClosure callback) {
    show_bubble_callback_for_testing_ = std::move(callback);
  }
  bool CloseOnDeactivateForTesting() const {
    return close_on_deactivate_for_testing_;
  }
  void SetSkipDelegateForTesting(bool skip) {
    skip_delegate_for_testing_ = skip;
  }
#endif

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveWalletTabHelperUnitTest, GetApproveBubbleURL);
  friend class content::WebContentsUserData<BraveWalletTabHelper>;
#if !defined(OS_ANDROID) && !defined(OS_IOS)
  GURL GetBubbleURL();
  base::OnceClosure show_bubble_callback_for_testing_;
  bool close_on_deactivate_for_testing_ = true;
  bool is_showing_bubble_for_testing_ = false;
  bool skip_delegate_for_testing_ = false;
  GURL GetApproveBubbleURL();
  std::unique_ptr<WalletBubbleManagerDelegate> wallet_bubble_manager_delegate_;
#endif
  raw_ptr<content::WebContents> web_contents_ = nullptr;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_TAB_HELPER_H_
