/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_helper.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "components/content_settings/core/common/content_settings.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/visibility.h"
#include "content/public/browser/web_contents.h"

namespace brave_wallet {

namespace {

bool IsAccountAllowed(const std::vector<std::string>& allowed_accounts,
                      const std::string& account) {
  for (const auto& allowed_account : allowed_accounts) {
    if (base::CompareCaseInsensitiveASCII(account, allowed_account) == 0)
      return true;
  }
  return false;
}

void OnRequestPermissions(
    const std::vector<std::string>& accounts,
    BraveWalletProviderDelegate::RequestPermissionsCallback callback,
    const std::vector<blink::mojom::PermissionStatus>& responses) {
  DCHECK(responses.empty() || responses.size() == accounts.size());

  std::vector<std::string> granted_accounts;
  for (size_t i = 0; i < responses.size(); i++) {
    if (responses[i] == blink::mojom::PermissionStatus::GRANTED) {
      granted_accounts.push_back(accounts[i]);
    }
  }
  // The responses array will be empty if operation failed.
  if (responses.empty()) {
    std::move(callback).Run(mojom::RequestPermissionsError::kInternal,
                            absl::nullopt);
  } else {
    std::move(callback).Run(mojom::RequestPermissionsError::kNone,
                            granted_accounts);
  }
}

void OnIsAccountAllowed(
    const std::string& account,
    BraveWalletProviderDelegate::IsAccountAllowedCallback callback,
    bool success,
    const std::vector<std::string>& allowed_accounts) {
  bool allowed = IsAccountAllowed(allowed_accounts, account);

  std::move(callback).Run(allowed);
}

absl::optional<permissions::RequestType> CoinTypeToPermissionRequestType(
    mojom::CoinType coin_type) {
  switch (coin_type) {
    case mojom::CoinType::ETH:
      return permissions::RequestType::kBraveEthereum;
    case mojom::CoinType::SOL:
      return permissions::RequestType::kBraveSolana;
    default:
      return absl::nullopt;
  }
}

}  // namespace

BraveWalletProviderDelegateImpl::BraveWalletProviderDelegateImpl(
    content::WebContents* web_contents,
    content::RenderFrameHost* const render_frame_host)
    : WebContentsObserver(web_contents),
      web_contents_(web_contents),
      host_id_(render_frame_host->GetGlobalId()),
      weak_ptr_factory_(this) {}

BraveWalletProviderDelegateImpl::~BraveWalletProviderDelegateImpl() = default;

url::Origin BraveWalletProviderDelegateImpl::GetOrigin() const {
  auto* rfh = content::RenderFrameHost::FromID(host_id_);
  return rfh ? rfh->GetLastCommittedOrigin() : url::Origin();
}

bool BraveWalletProviderDelegateImpl::IsTabVisible() {
  return web_contents_
             ? web_contents_->GetVisibility() == content::Visibility::VISIBLE
             : false;
}

void BraveWalletProviderDelegateImpl::ShowPanel() {
  ::brave_wallet::ShowPanel(web_contents_);
}

void BraveWalletProviderDelegateImpl::WalletInteractionDetected() {
  ::brave_wallet::WalletInteractionDetected(web_contents_);
}

void BraveWalletProviderDelegateImpl::ShowWalletOnboarding() {
  ::brave_wallet::ShowWalletOnboarding(web_contents_);
}

void BraveWalletProviderDelegateImpl::GetAllowedAccounts(
    mojom::CoinType type,
    const std::vector<std::string>& accounts,
    GetAllowedAccountsCallback callback) {
  auto permission = CoinTypeToPermissionType(type);
  if (!permission) {
    std::move(callback).Run(false, std::vector<std::string>());
    return;
  }
  permissions::BraveWalletPermissionContext::GetAllowedAccounts(
      *permission, content::RenderFrameHost::FromID(host_id_), accounts,
      std::move(callback));
}

void BraveWalletProviderDelegateImpl::RequestPermissions(
    mojom::CoinType type,
    const std::vector<std::string>& accounts,
    RequestPermissionsCallback callback) {
  if (!IsWeb3NotificationAllowed()) {
    std::move(callback).Run(mojom::RequestPermissionsError::kNone,
                            std::vector<std::string>());
    return;
  }
  auto request_type = CoinTypeToPermissionRequestType(type);
  auto permission = CoinTypeToPermissionType(type);
  if (!request_type || !permission) {
    std::move(callback).Run(mojom::RequestPermissionsError::kInternal,
                            absl::nullopt);
    return;
  }
  // Check if there's already a permission request in progress
  auto* rfh = content::RenderFrameHost::FromID(host_id_);
  if (rfh && permissions::BraveWalletPermissionContext::HasRequestsInProgress(
                 rfh, *request_type)) {
    std::move(callback).Run(mojom::RequestPermissionsError::kRequestInProgress,
                            absl::nullopt);
    return;
  }

  permissions::BraveWalletPermissionContext::RequestPermissions(
      *permission, content::RenderFrameHost::FromID(host_id_), accounts,
      base::BindOnce(&OnRequestPermissions, accounts, std::move(callback)));
}

void BraveWalletProviderDelegateImpl::IsAccountAllowed(
    mojom::CoinType type,
    const std::string& account,
    IsAccountAllowedCallback callback) {
  auto permission = CoinTypeToPermissionType(type);
  if (!permission) {
    std::move(callback).Run(false);
    return;
  }

  permissions::BraveWalletPermissionContext::GetAllowedAccounts(
      *permission, content::RenderFrameHost::FromID(host_id_), {account},
      base::BindOnce(&OnIsAccountAllowed, account, std::move(callback)));
}

void BraveWalletProviderDelegateImpl::WebContentsDestroyed() {
  web_contents_ = nullptr;
}

}  // namespace brave_wallet
