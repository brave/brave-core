/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"

#include <string>
#include <utility>
#include <vector>

#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_wallet/browser/ethereum_permission_utils.h"
#include "components/permissions/permission_request.h"
#include "components/permissions/permission_request_manager.h"
#include "components/permissions/request_type.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/web_contents.h"

#if !defined(OS_ANDROID) && !defined(OS_IOS)
#include "brave/browser/ui/brave_wallet/wallet_bubble_manager_delegate.h"
#endif

namespace brave_wallet {

BraveWalletTabHelper::BraveWalletTabHelper(content::WebContents* web_contents)
    : web_contents_(web_contents) {}

BraveWalletTabHelper::~BraveWalletTabHelper() {
#if !defined(OS_ANDROID) && !defined(OS_IOS)
  if (IsShowingBubble())
    CloseBubble();
#endif  // !defined(OS_ANDROID) && !defined(OS_IOS)
}

#if !defined(OS_ANDROID) && !defined(OS_IOS)
void BraveWalletTabHelper::ClosePanelOnDeactivate(bool close) {
  if (wallet_bubble_manager_delegate_)
    wallet_bubble_manager_delegate_->CloseOnDeactivate(close);
  close_on_deactivate_for_testing_ = close;
}

void BraveWalletTabHelper::ShowBubble() {
  wallet_bubble_manager_delegate_ =
      WalletBubbleManagerDelegate::Create(web_contents_, GetBubbleURL());
  wallet_bubble_manager_delegate_->ShowBubble();
  if (show_bubble_callback_for_testing_)
    std::move(show_bubble_callback_for_testing_).Run();
}

void BraveWalletTabHelper::ShowApproveWalletBubble() {
  // If the Wallet page is open, then it will try to open the UI.
  // But the user may have already had the panel UI opened.
  // We want to avoid a hiding / showing of the panel in that case.
  if (IsShowingBubble())
    return;
  wallet_bubble_manager_delegate_ =
      WalletBubbleManagerDelegate::Create(web_contents_, GetApproveBubbleURL());
  wallet_bubble_manager_delegate_->ShowBubble();
}

void BraveWalletTabHelper::CloseBubble() {
  if (wallet_bubble_manager_delegate_)
    wallet_bubble_manager_delegate_->CloseBubble();
}

bool BraveWalletTabHelper::IsShowingBubble() {
  return wallet_bubble_manager_delegate_ &&
         wallet_bubble_manager_delegate_->IsShowingBubble();
}

bool BraveWalletTabHelper::IsBubbleClosedForTesting() {
  return wallet_bubble_manager_delegate_ &&
         wallet_bubble_manager_delegate_->IsBubbleClosedForTesting();
}

GURL BraveWalletTabHelper::GetBubbleURL() {
  auto* manager =
      permissions::PermissionRequestManager::FromWebContents(web_contents_);
  DCHECK(manager);

  GURL webui_url = GURL(kBraveUIWalletPanelURL);

  // General panel UI if no pending ethereum permission request.
  // Only check the first entry because it will not be grouped with other
  // types.
  if (manager->Requests().empty() ||
      manager->Requests()[0]->request_type() !=
          permissions::RequestType::kBraveEthereum)
    return webui_url;

  // Handle ConnectWithSite (ethereum permission) request.
  std::vector<std::string> accounts;
  std::string requesting_origin;
  for (auto* request : manager->Requests()) {
    std::string account;
    if (!brave_wallet::ParseRequestingOriginFromSubRequest(
            request->requesting_origin(), &requesting_origin, &account)) {
      continue;
    }
    accounts.push_back(account);
  }
  DCHECK(!accounts.empty());

  int32_t tab_id = sessions::SessionTabHelper::IdForTab(web_contents_).id();
  webui_url = brave_wallet::GetConnectWithSiteWebUIURL(
      webui_url, tab_id, accounts, requesting_origin);
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
  url::Replacements<char> replacements;
  const std::string ref = "approveTransaction";
  replacements.SetRef(ref.c_str(), url::Component(0, ref.size()));
  return webui_url.ReplaceComponents(replacements);
}

#endif  // !defined(OS_ANDROID) && !defined(OS_IOS)

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveWalletTabHelper);

}  // namespace brave_wallet
