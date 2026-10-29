/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_service_delegate_impl_android.h"

#include <utility>

#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "content/public/browser/browser_context.h"

namespace brave_wallet {

BraveWalletServiceDelegateImpl::BraveWalletServiceDelegateImpl(
    content::BrowserContext* context)
    : BraveWalletServiceDelegateBase(context) {}

BraveWalletServiceDelegateImpl::~BraveWalletServiceDelegateImpl() = default;

bool BraveWalletServiceDelegateImpl::AddPermission(mojom::CoinType coin,
                                                   const url::Origin& origin,
                                                   const std::string& account) {
  auto type = CoinTypeToPermissionType(coin);
  if (!type) {
    return false;
  }

  return permissions::BraveWalletPermissionContext::AddPermission(
      *type, context_, origin, account);
}

void BraveWalletServiceDelegateImpl::GetWebSitesWithPermission(
    mojom::CoinType coin,
    GetWebSitesWithPermissionCallback callback) {
  auto type = CoinTypeToPermissionType(coin);
  if (!type) {
    std::move(callback).Run({});
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
