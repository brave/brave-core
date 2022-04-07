/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_service_delegate_impl_android.h"

#include <utility>

#include "brave/components/permissions/contexts/brave_ethereum_permission_context.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace brave_wallet {

BraveWalletServiceDelegateImpl::BraveWalletServiceDelegateImpl(
    content::BrowserContext* context)
    : context_(context), weak_ptr_factory_(this) {}

BraveWalletServiceDelegateImpl::~BraveWalletServiceDelegateImpl() = default;

void BraveWalletServiceDelegateImpl::AddEthereumPermission(
    const url::Origin& origin,
    const std::string& account,
    AddEthereumPermissionCallback callback) {
  bool success =
      permissions::BraveEthereumPermissionContext::AddEthereumPermission(
          context_, origin, account);
  std::move(callback).Run(success);
}

void BraveWalletServiceDelegateImpl::HasEthereumPermission(
    const url::Origin& origin,
    const std::string& account,
    HasEthereumPermissionCallback callback) {
  bool has_permission = false;
  bool success =
      permissions::BraveEthereumPermissionContext::HasEthereumPermission(
          context_, origin, account, &has_permission);
  std::move(callback).Run(success, has_permission);
}

void BraveWalletServiceDelegateImpl::ResetEthereumPermission(
    const url::Origin& origin,
    const std::string& account,
    ResetEthereumPermissionCallback callback) {
  bool success =
      permissions::BraveEthereumPermissionContext::ResetEthereumPermission(
          context_, origin, account);
  std::move(callback).Run(success);
}

}  // namespace brave_wallet
