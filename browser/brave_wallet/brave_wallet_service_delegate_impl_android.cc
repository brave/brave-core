/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_service_delegate_impl_android.h"

#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace brave_wallet {

BraveWalletServiceDelegateImpl::BraveWalletServiceDelegateImpl(
    content::BrowserContext* context)
    : context_(context), weak_ptr_factory_(this) {}

BraveWalletServiceDelegateImpl::~BraveWalletServiceDelegateImpl() = default;

void BraveWalletServiceDelegateImpl::AddPermission(
    mojom::CoinType coin,
    const url::Origin& origin,
    const std::string& account,
    AddPermissionCallback callback) {
  auto type = CoinTypeToPermissionType(coin);
  if (!type) {
    std::move(callback).Run(false);
    return;
  }
  bool success = permissions::BraveWalletPermissionContext::AddPermission(
      *type, context_, origin, account);
  std::move(callback).Run(success);
}

void BraveWalletServiceDelegateImpl::HasPermission(
    mojom::CoinType coin,
    const url::Origin& origin,
    const std::string& account,
    HasPermissionCallback callback) {
  bool has_permission = false;
  auto type = CoinTypeToPermissionType(coin);
  if (!type) {
    std::move(callback).Run(false, has_permission);
    return;
  }
  bool success = permissions::BraveWalletPermissionContext::HasPermission(
      *type, context_, origin, account, &has_permission);
  std::move(callback).Run(success, has_permission);
}

void BraveWalletServiceDelegateImpl::ResetPermission(
    mojom::CoinType coin,
    const url::Origin& origin,
    const std::string& account,
    ResetPermissionCallback callback) {
  auto type = brave_wallet::CoinTypeToPermissionType(coin);
  if (!type) {
    std::move(callback).Run(false);
    return;
  }
  bool success = permissions::BraveWalletPermissionContext::ResetPermission(
      *type, context_, origin, account);
  std::move(callback).Run(success);
}

void BraveWalletServiceDelegateImpl::IsPermissionDenied(
    mojom::CoinType coin,
    const url::Origin& origin,
    IsPermissionDeniedCallback callback) {
  auto type = CoinTypeToPermissionType(coin);
  if (!type) {
    std::move(callback).Run(false);
    return;
  }
  std::move(callback).Run(
      permissions::BraveWalletPermissionContext::IsPermissionDenied(
          *type, context_, origin));
}

void BraveWalletServiceDelegateImpl::GetWebSitesWithPermission(
    mojom::CoinType coin,
    GetWebSitesWithPermissionCallback callback) {
  std::vector<std::string> result;
  auto type = CoinTypeToPermissionType(coin);
  if (!type) {
    std::move(callback).Run(result);
    return;
  }
  std::move(callback).Run(
      permissions::BraveWalletPermissionContext::GetWebSitesWithPermission(
          *type, context_));
}

void BraveWalletServiceDelegateImpl::ResetWebSitePermission(
    mojom::CoinType coin,
    const std::string& formed_website,
    ResetWebSitePermissionCallback callback) {
  auto type = CoinTypeToPermissionType(coin);
  if (!type) {
    std::move(callback).Run(false);
    return;
  }
  std::move(callback).Run(
      permissions::BraveWalletPermissionContext::ResetWebSitePermission(
          *type, context_, formed_website));
}

}  // namespace brave_wallet
