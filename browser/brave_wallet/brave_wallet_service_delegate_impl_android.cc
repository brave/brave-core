/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_service_delegate_impl_android.h"

#include <utility>

#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "components/permissions/permission_util.h"
#include "components/permissions/request_type.h"
#include "content/public/browser/browser_context.h"

namespace brave_wallet {

BraveWalletServiceDelegateImpl::BraveWalletServiceDelegateImpl(
    content::BrowserContext* context)
    : BraveWalletServiceDelegateBase(context) {}

BraveWalletServiceDelegateImpl::~BraveWalletServiceDelegateImpl() = default;

bool BraveWalletServiceDelegateImpl::AddPermission(mojom::CoinType coin,
                                                   const url::Origin& origin,
                                                   const std::string& account) {
  auto content_settings_type =
      brave_wallet::CoinTypeToContentSettingsType(coin);
  if (!content_settings_type) {
    return false;
  }

  return permissions::BraveWalletPermissionContext::AddPermission(
      permissions::PermissionUtil::ContentSettingsTypeToPermissionType(
          *content_settings_type),
      context_, origin, account);
}

void BraveWalletServiceDelegateImpl::GetWebSitesWithPermission(
    mojom::CoinType coin,
    GetWebSitesWithPermissionCallback callback) {
  auto content_settings_type =
      brave_wallet::CoinTypeToContentSettingsType(coin);
  if (!content_settings_type) {
    std::move(callback).Run({});
    return;
  }
  std::move(callback).Run(
      permissions::BraveWalletPermissionContext::GetWebSitesWithPermission(
          permissions::PermissionUtil::ContentSettingsTypeToPermissionType(
              *content_settings_type),
          context_));
}

void BraveWalletServiceDelegateImpl::ResetWebSitePermission(
    mojom::CoinType coin,
    const std::string& formed_website,
    ResetWebSitePermissionCallback callback) {
  auto content_settings_type =
      brave_wallet::CoinTypeToContentSettingsType(coin);
  if (!content_settings_type) {
    std::move(callback).Run(false);
    return;
  }
  std::move(callback).Run(
      permissions::BraveWalletPermissionContext::ResetWebSitePermission(
          permissions::PermissionUtil::ContentSettingsTypeToPermissionType(
              *content_settings_type),
          context_, formed_website));
}

}  // namespace brave_wallet
