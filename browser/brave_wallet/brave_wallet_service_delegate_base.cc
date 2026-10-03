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
#include "chrome/browser/profiles/profile.h"
#include "components/permissions/permission_util.h"
#include "content/public/browser/browser_context.h"

#if BUILDFLAG(ENABLE_SNAP)
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "brave/components/brave_wallet/content/browser/snap/hidden_web_contents_snap_host_bridge_controller.h"
#include "url/gurl.h"
#endif

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

#if BUILDFLAG(ENABLE_SNAP)
std::unique_ptr<SnapHostBridgeController>
BraveWalletServiceDelegateBase::CreateSnapHostBridgeController() {
  switch (GetSnapExecutionEnvironment()) {
    case SnapExecutionEnvironment::kWalletPage:
      return BraveWalletServiceDelegate::CreateSnapHostBridgeController();
    case SnapExecutionEnvironment::kHiddenWebContents:
      return std::make_unique<HiddenWebContentsSnapHostBridgeController>(
          context_, GURL(kBraveUIWalletSnapHostURL), /*start_host=*/true);
    case SnapExecutionEnvironment::kHostPageDebug:
      return std::make_unique<HiddenWebContentsSnapHostBridgeController>(
          context_, GURL(kBraveUIWalletSnapHostURL), /*start_host=*/false);
  }
}
#endif

}  // namespace brave_wallet
