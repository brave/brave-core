/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_service_delegate_base.h"

#include "base/auto_reset.h"
#include "base/command_line.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/permissions/permission_util.h"
#include "content/public/browser/browser_context.h"

namespace {
bool g_enable_wallet_autolock = true;
}

namespace brave_wallet {

BraveWalletServiceDelegateBase::BraveWalletServiceDelegateBase(
    content::BrowserContext* context)
    : context_(context) {
  wallet_base_directory_ = context->GetPath().AppendASCII(kWalletBaseDirectory);
  is_private_window_ =
      Profile::FromBrowserContext(context)->IsIncognitoProfile();
  auto* map = HostContentSettingsMapFactory::GetForProfile(context);
  DCHECK(map);
  content_settings_observation_.Observe(map);
}

BraveWalletServiceDelegateBase::~BraveWalletServiceDelegateBase() = default;

bool BraveWalletServiceDelegateBase::HasPermission(mojom::CoinType coin,
                                                   const url::Origin& origin,
                                                   const std::string& account) {
  bool has_permission = false;
  auto type = CoinTypeToPermissionType(coin);
  if (!type) {
    return false;
  }

  bool success = permissions::BraveWalletPermissionContext::HasPermission(
      *type, context_, origin, account, &has_permission);
  return success && has_permission;
}

bool BraveWalletServiceDelegateBase::ResetPermission(
    mojom::CoinType coin,
    const url::Origin& origin,
    const std::string& account) {
  auto type = CoinTypeToPermissionType(coin);
  if (!type) {
    return false;
  }

  return permissions::BraveWalletPermissionContext::ResetPermission(
      *type, context_, origin, account);
}

void BraveWalletServiceDelegateBase::ResetPermissionsForAccount(
    mojom::CoinType coin,
    const std::string& account) {
  auto type = CoinTypeToPermissionType(coin);
  if (!type) {
    return;
  }

  const auto content_settings_type =
      permissions::PermissionUtil::PermissionTypeToContentSettingsTypeSafe(
          *type);
  permissions::BraveWalletPermissionContext::ResetPermissionsForAccount(
      context_, content_settings_type, account);
}

bool BraveWalletServiceDelegateBase::IsPermissionDenied(
    mojom::CoinType coin,
    const url::Origin& origin) {
  auto type = CoinTypeToPermissionType(coin);
  if (!type) {
    return false;
  }

  return permissions::BraveWalletPermissionContext::IsPermissionDenied(
      *type, context_, origin);
}

void BraveWalletServiceDelegateBase::ResetAllPermissions() {
  permissions::BraveWalletPermissionContext::ResetAllPermissions(context_);
}

base::FilePath BraveWalletServiceDelegateBase::GetWalletBaseDirectory() {
  return wallet_base_directory_;
}

bool BraveWalletServiceDelegateBase::IsPrivateWindow() {
  return is_private_window_;
}

bool BraveWalletServiceDelegateBase::IsAutolockEnabled() {
  return g_enable_wallet_autolock;
}

// static
base::AutoReset<bool>
BraveWalletServiceDelegateBase::GetScopedDisableAutolockForTesting() {
  return {&g_enable_wallet_autolock, false};
}

void BraveWalletServiceDelegateBase::OnContentSettingChanged(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type) {
  if (content_type != ContentSettingsType::BRAVE_ETHEREUM &&
      content_type != ContentSettingsType::BRAVE_SOLANA &&
      content_type != ContentSettingsType::BRAVE_CARDANO) {
    return;
  }
  if (content_setting_changed_callback_) {
    content_setting_changed_callback_.Run();
  }
}

void BraveWalletServiceDelegateBase::SetContentSettingChangedCallback(
    base::RepeatingClosure callback) {
  content_setting_changed_callback_ = std::move(callback);
}

}  // namespace brave_wallet
