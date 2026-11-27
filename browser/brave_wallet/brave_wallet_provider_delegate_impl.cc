/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl.h"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_helper.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/visibility.h"
#include "content/public/browser/web_contents.h"

namespace brave_wallet {

namespace {

void OnRequestPermissions(
    BraveWalletProviderDelegate::RequestPermissionsCallback callback,
    std::vector<mojom::AccountIdPtr> queried_accounts,
    std::vector<std::string> allowed_identifiers) {
  std::vector<mojom::AccountIdPtr> allowed_accounts;
  for (auto& account : queried_accounts) {
    if (std::ranges::contains(allowed_identifiers,
                              GetAccountPermissionIdentifier(account))) {
      allowed_accounts.push_back(std::move(account));
    }
  }
  std::move(callback).Run(mojom::RequestPermissionsError::kNone,
                          std::move(allowed_accounts));
}

}  // namespace

BraveWalletProviderDelegateImpl::BraveWalletProviderDelegateImpl(
    content::WebContents* web_contents,
    content::GlobalRenderFrameHostId render_frame_host_id)
    : WebContentsObserver(web_contents), host_id_(render_frame_host_id) {}

BraveWalletProviderDelegateImpl::~BraveWalletProviderDelegateImpl() = default;

bool BraveWalletProviderDelegateImpl::IsTabVisible() {
  CHECK(web_contents());
  return web_contents()->GetVisibility() == content::Visibility::VISIBLE;
}

void BraveWalletProviderDelegateImpl::ShowPanel(const url::Origin& origin) {
  ::brave_wallet::ShowPanel(web_contents());
}

void BraveWalletProviderDelegateImpl::ShowWalletBackup() {
  ::brave_wallet::ShowWalletBackup();
}

void BraveWalletProviderDelegateImpl::UnlockWallet() {
  ::brave_wallet::UnlockWallet();
}

void BraveWalletProviderDelegateImpl::WalletInteractionDetected() {
  ::brave_wallet::WalletInteractionDetected(web_contents());
}

void BraveWalletProviderDelegateImpl::ShowAccountCreation(
    mojom::CoinType type,
    const url::Origin& origin) {
  DCHECK(type == mojom::CoinType::SOL || type == mojom::CoinType::ADA ||
         type == mojom::CoinType::DOT);
  ::brave_wallet::ShowAccountCreation(web_contents(), type);
}

std::vector<mojom::AccountIdPtr>
BraveWalletProviderDelegateImpl::GetAllowedAccounts(
    const std::vector<mojom::AccountIdPtr>& accounts) {
  auto type = GetAccountsCoinType(accounts);
  if (IsPermissionDenied(type)) {
    return std::vector<mojom::AccountIdPtr>();
  }

  auto permission = CoinTypeToPermissionType(type);
  if (!permission) {
    return std::vector<mojom::AccountIdPtr>();
  }

  return permissions::BraveWalletPermissionContext::GetAllowedAccounts(
      *permission, content::RenderFrameHost::FromID(host_id_), accounts);
}

void BraveWalletProviderDelegateImpl::RequestPermissions(
    const std::vector<mojom::AccountIdPtr>& accounts,
    const url::Origin& origin,
    RequestPermissionsCallback callback) {
  auto type = GetAccountsCoinType(accounts);
  if (!IsWeb3NotificationAllowed()) {
    std::move(callback).Run(mojom::RequestPermissionsError::kNone, {});
    return;
  }
  auto request_type = CoinTypeToPermissionRequestType(type);
  auto permission = CoinTypeToPermissionType(type);
  if (!request_type || !permission) {
    std::move(callback).Run(mojom::RequestPermissionsError::kInternal, {});
    return;
  }
  // Check if there's already a permission request in progress
  auto* rfh = content::RenderFrameHost::FromID(host_id_);
  if (rfh && permissions::BraveWalletPermissionContext::HasRequestsInProgress(
                 rfh, *request_type)) {
    std::move(callback).Run(mojom::RequestPermissionsError::kRequestInProgress,
                            {});
    return;
  }

  permissions::BraveWalletPermissionContext::RequestWalletPermissions(
      GetAccountPermissionIdentifiers(accounts), *permission, origin,
      content::RenderFrameHost::FromID(host_id_),
      base::BindOnce(&OnRequestPermissions, std::move(callback),
                     CloneVector(accounts)));
}

bool BraveWalletProviderDelegateImpl::IsAccountAllowed(
    const mojom::AccountIdPtr& account) {
  auto permission = CoinTypeToPermissionType(account->coin);
  if (!permission) {
    return false;
  }

  std::vector<mojom::AccountIdPtr> accounts;
  accounts.push_back(account.Clone());
  const auto allowed_accounts =
      permissions::BraveWalletPermissionContext::GetAllowedAccounts(
          *permission, content::RenderFrameHost::FromID(host_id_), accounts);

  return std::ranges::contains(allowed_accounts, account);
}

bool BraveWalletProviderDelegateImpl::IsPermissionDenied(mojom::CoinType type) {
  auto permission = CoinTypeToPermissionType(type);
  if (!permission) {
    return false;
  }

  auto* rfh = content::RenderFrameHost::FromID(host_id_);
  if (!rfh) {
    return false;
  }

  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);

  return permissions::BraveWalletPermissionContext::IsPermissionDenied(
      *permission, web_contents->GetBrowserContext(),
      url::Origin::Create(rfh->GetLastCommittedURL()));
}

}  // namespace brave_wallet
