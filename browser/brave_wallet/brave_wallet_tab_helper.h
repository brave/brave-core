/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_TAB_HELPER_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_TAB_HELPER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "content/public/browser/web_contents_user_data.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
struct GlobalRenderFrameHostId;
class WebContents;

}  // namespace content

namespace brave_wallet {

class WalletBubbleManagerDelegate;

class BraveWalletTabHelper
    : public content::WebContentsUserData<BraveWalletTabHelper> {
 public:
  explicit BraveWalletTabHelper(content::WebContents* web_contents);
  ~BraveWalletTabHelper() override;

  void AddSolanaConnectedAccount(const content::GlobalRenderFrameHostId& id,
                                 const std::string& account);
  void RemoveSolanaConnectedAccount(const content::GlobalRenderFrameHostId& id,
                                    const std::string& account);
  bool IsSolanaAccountConnected(const content::GlobalRenderFrameHostId& id,
                                const std::string& account);
  void ClearSolanaConnectedAccounts(const content::GlobalRenderFrameHostId& id);

#if !BUILDFLAG(IS_ANDROID)
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
  FRIEND_TEST_ALL_PREFIXES(BraveWalletTabHelperUnitTest,
                           SolanaConnectedAccount);
  friend class content::WebContentsUserData<BraveWalletTabHelper>;

  // This set is used to maintain connected status for each frame, it is a
  // separate non persistent status from site permission. It depends on if a
  // site successfully call connect or not, calling disconnect will remove
  // itself from this set. Note that site permission is required for a site to
  // do connect, if an user reject the connect request, connect would fail. On
  // the other hand, if the user approve the connect request, site permission
  // will be saved and future connect from the same site will not ask user for
  // permission again until the permission is removed.
  // Each RenderFrameHost has its own connection set.
  base::flat_map<content::GlobalRenderFrameHostId, base::flat_set<std::string>>
      solana_connected_accounts_;
#if !BUILDFLAG(IS_ANDROID)
  GURL GetBubbleURL();
  base::OnceClosure show_bubble_callback_for_testing_;
  bool close_on_deactivate_for_testing_ = true;
  bool is_showing_bubble_for_testing_ = false;
  bool skip_delegate_for_testing_ = false;
  GURL GetApproveBubbleURL();
  std::unique_ptr<WalletBubbleManagerDelegate> wallet_bubble_manager_delegate_;
#endif

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_TAB_HELPER_H_
