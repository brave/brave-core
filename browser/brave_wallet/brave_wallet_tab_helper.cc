/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"

#include <string>
#include <utility>
#include <vector>

#include "base/strings/stringprintf.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_wallet/browser/ethereum_permission_utils.h"
#include "brave/components/brave_wallet/common/web3_provider_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/permissions/permission_request.h"
#include "components/permissions/permission_request_manager.h"
#include "components/permissions/request_type.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(OS_ANDROID) && !defined(OS_IOS)
#include "brave/browser/ui/brave_wallet/wallet_bubble_manager_delegate.h"
#endif

namespace {

#if !defined(OS_ANDROID) && !defined(OS_IOS)
GURL GetAddEthereumChainPayloadWebUIURL(const GURL& webui_base_url,
                                        int32_t tab_id,
                                        const std::string& payload) {
  DCHECK(webui_base_url.is_valid() && tab_id > 0 && !payload.empty());

  std::vector<std::string> query_parts;
  query_parts.push_back(base::StringPrintf("payload=%s", payload.c_str()));
  query_parts.push_back(base::StringPrintf("tabId=%d", tab_id));
  std::string query_str = base::JoinString(query_parts, "&");
  url::Replacements<char> replacements;
  replacements.SetQuery(query_str.c_str(), url::Component(0, query_str.size()));
  const std::string kAddEthereumChain = "addEthereumChain";
  replacements.SetRef(kAddEthereumChain.c_str(),
                      url::Component(0, kAddEthereumChain.size()));
  return webui_base_url.ReplaceComponents(replacements);
}
#endif

}  // namespace

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

void BraveWalletTabHelper::UserRequestCompleted(const std::string& chain_id,
                                                const std::string& error) {
  DCHECK(request_callbacks_.contains(chain_id));
  std::move(request_callbacks_[chain_id]).Run(error);
  request_callbacks_.erase(chain_id);
}

void BraveWalletTabHelper::RequestUserApproval(
    const std::string& chain_id,
    const std::string& payload,
    RequestEthereumChainCallback callback) {
  std::vector<std::string> accounts;
  int32_t tab_id = sessions::SessionTabHelper::IdForTab(web_contents_).id();
  GURL webui_url = GetAddEthereumChainPayloadWebUIURL(
      GURL(kBraveUIWalletPanelURL), tab_id, payload);
  DCHECK(webui_url.is_valid());
  if (request_callbacks_.contains(chain_id)) {
    UserRequestCompleted(chain_id, std::string());
    return;
  }
  request_callbacks_[chain_id] = std::move(callback);
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
