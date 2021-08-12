/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"

#include <string>
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

BraveWalletTabHelper::~BraveWalletTabHelper() = default;

#if !defined(OS_ANDROID) && !defined(OS_IOS)
void BraveWalletTabHelper::ShowBubble() {
  wallet_bubble_manager_delegate_ =
      WalletBubbleManagerDelegate::Create(web_contents_, GetBubbleURL());
  wallet_bubble_manager_delegate_->ShowBubble();
}

void BraveWalletTabHelper::UserRequestApproved(const std::string& requestData) {
  size_t hash = base::FastHash(base::as_bytes(base::make_span(requestData)));
  DCHECK(request_callbacks_.count(hash));
  std::move(request_callbacks_[hash]).Run(std::vector<std::string>(1,{"done"}));
  request_callbacks_.erase(hash);
}

void BraveWalletTabHelper::RequestUserApproval(const std::string& requestData,
    BraveWalletProviderDelegate::RequestEthereumPermissionsCallback callback) {
  std::string requesting_origin;
  std::vector<std::string> accounts;
  auto* manager =
      permissions::PermissionRequestManager::FromWebContents(web_contents_);
  DCHECK(manager);

  requesting_origin = "someorigin";

  int32_t tab_id = sessions::SessionTabHelper::IdForTab(web_contents_).id();
  GURL webui_url = brave_wallet::GetConnectWithPayloadWebUIURL(
      GetBubbleURL(), tab_id, requesting_origin, requestData);
  DCHECK(webui_url.is_valid());
  
  size_t hash = base::FastHash(base::as_bytes(base::make_span(requestData)));
  DCHECK(!request_callbacks_.count(hash));
  request_callbacks_[hash] = std::move(callback);

  wallet_bubble_manager_delegate_ =
      WalletBubbleManagerDelegate::Create(web_contents_, webui_url);
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
      manager->Requests()[0]->GetRequestType() !=
          permissions::RequestType::kBraveEthereum)
    return webui_url;

  // Handle ConnectWithSite (ethereum permission) request.
  std::vector<std::string> accounts;
  std::string requesting_origin;
  for (auto* request : manager->Requests()) {
    std::string account;
    if (!brave_wallet::ParseRequestingOriginFromSubRequest(
            request->GetOrigin(), &requesting_origin, &account)) {
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
#endif  // !defined(OS_ANDROID) && !defined(OS_IOS)

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveWalletTabHelper)

}  // namespace brave_wallet
