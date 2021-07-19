/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl.h"

#include <string>
#include <utility>
#include <vector>

#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/ethereum_permission_utils.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/permissions/permission_manager.h"
#include "components/permissions/permissions_client.h"
#include "content/public/browser/web_contents.h"

namespace brave_wallet {

namespace {

void OnRequestEthereumPermissions(
    BraveWalletProviderDelegate::RequestEthereumPermissionsCallback callback,
    const std::vector<ContentSetting>& responses) {
  bool success = false;
  for (auto response : responses) {
    if (response == CONTENT_SETTING_ALLOW) {
      success = true;
      break;
    }
  }

  std::move(callback).Run(success);
}

}  // namespace

BraveWalletProviderDelegateImpl::BraveWalletProviderDelegateImpl(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {}

void BraveWalletProviderDelegateImpl::ShowConnectToSiteUI() {
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents_);
  brave::ShowWalletBubble(browser);
}

void BraveWalletProviderDelegateImpl::RequestEthereumPermissions(
    RequestEthereumPermissionsCallback callback) {
  permissions::PermissionManager* permission_manager =
      permissions::PermissionsClient::Get()->GetPermissionManager(
          web_contents_->GetBrowserContext());
  if (!permission_manager) {
    std::move(callback).Run(false);
    return;
  }

  auto* service = BraveWalletServiceFactory::GetInstance()->GetForContext(
      web_contents_->GetBrowserContext());
  auto* keyring_controller = service->keyring_controller();
  auto* default_keyring = keyring_controller->GetDefaultKeyring();
  std::vector<std::string> addresses;
  if (default_keyring) {
    addresses = default_keyring->GetAccounts();
  }

  // To support ethereum permission to be per Ethereum account per site, we map
  // each account address to one ethereum permission request. And the requests
  // will have different origins which includes the address information. Here
  // we first get a concatenated origin to include information of all wallet
  // addresses, then adjust the origin of each request later in the process
  // because PermissionManager::RequestPermissions only accepts a single origin
  // parameter to be passes in.
  GURL origin;
  if (!GetConcatOriginFromWalletAddresses(
          web_contents_->GetLastCommittedURL().GetOrigin(), addresses,
          &origin)) {
    // Failed due to origin is invalid or addresses is empty.
    std::move(callback).Run(false);
    return;
  }
  std::vector<ContentSettingsType> types(addresses.size(),
                                         ContentSettingsType::BRAVE_ETHEREUM);
  permission_manager->RequestPermissions(
      types, web_contents_->GetMainFrame(), origin, true,
      base::BindOnce(&OnRequestEthereumPermissions, std::move(callback)));
}

}  // namespace brave_wallet
