/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"

#include <string>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/constants/webui_url_constants.h"
#include "components/permissions/permission_request.h"
#include "components/permissions/permission_request_manager.h"
#include "components/permissions/request_type.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/web_contents.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/brave_wallet/wallet_bubble_manager_delegate.h"
#endif

namespace brave_wallet {

BraveWalletTabHelper::BraveWalletTabHelper(content::WebContents* web_contents)
    : content::WebContentsUserData<BraveWalletTabHelper>(*web_contents) {}

BraveWalletTabHelper::~BraveWalletTabHelper() {
#if !BUILDFLAG(IS_ANDROID)
  if (IsShowingBubble()) {
    CloseBubble();
  }
#endif  // !BUILDFLAG(IS_ANDROID)
}

void BraveWalletTabHelper::AddSolanaConnectedAccount(
    const content::GlobalRenderFrameHostId& id,
    const std::string& account) {
  solana_connected_accounts_[id].insert(account);
}

void BraveWalletTabHelper::RemoveSolanaConnectedAccount(
    const content::GlobalRenderFrameHostId& id,
    const std::string& account) {
  auto it = solana_connected_accounts_.find(id);
  if (it == solana_connected_accounts_.end()) {
    return;
  }
  it->second.erase(account);
}

bool BraveWalletTabHelper::IsSolanaAccountConnected(
    const content::GlobalRenderFrameHostId& id,
    const std::string& account) {
  auto it = solana_connected_accounts_.find(id);
  return it == solana_connected_accounts_.end() ? false
                                                : it->second.contains(account);
}

void BraveWalletTabHelper::ClearSolanaConnectedAccounts(
    const content::GlobalRenderFrameHostId& id) {
  solana_connected_accounts_.erase(id);
}

#if !BUILDFLAG(IS_ANDROID)
void BraveWalletTabHelper::SetCloseOnDeactivate(bool close) {
  if (wallet_bubble_manager_delegate_) {
    wallet_bubble_manager_delegate_->CloseOnDeactivate(close);
  }
  close_on_deactivate_for_testing_ = close;
}

void BraveWalletTabHelper::ShowBubble() {
  if (skip_delegate_for_testing_) {
    is_showing_bubble_for_testing_ = true;
    return;
  }
  auto bubble_url = GetBubbleURL();
  if (!bubble_url.is_valid()) {
    return;
  }
  wallet_bubble_manager_delegate_ =
      WalletBubbleManagerDelegate::Create(&GetWebContents(), bubble_url);
  wallet_bubble_manager_delegate_->ShowBubble();
  if (show_bubble_callback_for_testing_) {
    std::move(show_bubble_callback_for_testing_).Run();
  }
}

void BraveWalletTabHelper::ShowApproveWalletBubble() {
  // If the Wallet page is open, then it will try to open the UI.
  // But the user may have already had the panel UI opened.
  // We want to avoid a hiding / showing of the panel in that case.
  if (IsShowingBubble()) {
    return;
  }
  wallet_bubble_manager_delegate_ = WalletBubbleManagerDelegate::Create(
      &GetWebContents(), GetApproveBubbleURL());
  wallet_bubble_manager_delegate_->ShowBubble();
}

void BraveWalletTabHelper::CloseBubble() {
  if (skip_delegate_for_testing_) {
    is_showing_bubble_for_testing_ = false;
    return;
  }
  if (wallet_bubble_manager_delegate_) {
    wallet_bubble_manager_delegate_->CloseBubble();
  }
}

bool BraveWalletTabHelper::IsShowingBubble() {
  if (skip_delegate_for_testing_) {
    return is_showing_bubble_for_testing_;
  }
  return wallet_bubble_manager_delegate_ &&
         wallet_bubble_manager_delegate_->IsShowingBubble();
}

bool BraveWalletTabHelper::IsBubbleClosedForTesting() {
  return wallet_bubble_manager_delegate_ &&
         wallet_bubble_manager_delegate_->IsBubbleClosedForTesting();
}

GURL BraveWalletTabHelper::GetBubbleURL() {
  auto* manager =
      permissions::PermissionRequestManager::FromWebContents(&GetWebContents());
  if (!manager) {
    return GURL();
  }

  GURL webui_url = GURL(kBraveUIWalletPanelURL);

  // General panel UI if no pending ethereum permission request.
  // Only check the first entry because it will not be grouped with other
  // types.
  if (manager->Requests().empty() ||
      (manager->Requests()[0]->request_type() !=
           permissions::RequestType::kBraveEthereum &&
       manager->Requests()[0]->request_type() !=
           permissions::RequestType::kBraveSolana)) {
    return webui_url;
  }

  // Handle ConnectWithSite (ethereum permission) request.
  std::vector<std::string> accounts;
  url::Origin requesting_origin;
  for (permissions::PermissionRequest* request : manager->Requests()) {
    std::string account;
    if (!brave_wallet::ParseRequestingOriginFromSubRequest(
            request->request_type(),
            url::Origin::Create(request->requesting_origin()),
            &requesting_origin, &account)) {
      continue;
    }
    accounts.push_back(account);
  }
  DCHECK(!accounts.empty());

  webui_url = brave_wallet::GetConnectWithSiteWebUIURL(webui_url, accounts,
                                                       requesting_origin);
  DCHECK(webui_url.is_valid());

  return webui_url;
}

content::WebContents* BraveWalletTabHelper::GetBubbleWebContentsForTesting() {
  return wallet_bubble_manager_delegate_->GetWebContentsForTesting();
}

const std::vector<int32_t>& BraveWalletTabHelper::GetPopupIdsForTesting() {
  return wallet_bubble_manager_delegate_->GetPopupIdsForTesting();
}

GURL BraveWalletTabHelper::GetApproveBubbleURL() {
  GURL webui_url = GURL(kBraveUIWalletPanelURL);
  GURL::Replacements replacements;
  const std::string ref = "approveTransaction";
  replacements.SetRefStr(ref);
  return webui_url.ReplaceComponents(replacements);
}

#endif  // !BUILDFLAG(IS_ANDROID)

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveWalletTabHelper);

}  // namespace brave_wallet
