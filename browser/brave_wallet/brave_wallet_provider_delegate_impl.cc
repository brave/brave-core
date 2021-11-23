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
#include "brave/browser/brave_wallet/keyring_controller_factory.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "brave/components/permissions/contexts/brave_ethereum_permission_context.h"
#include "components/content_settings/core/common/content_settings.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace brave_wallet {

namespace {

void OnRequestEthereumPermissions(
    const std::vector<std::string>& accounts,
    BraveWalletProviderDelegate::RequestEthereumPermissionsCallback callback,
    const std::vector<ContentSetting>& responses) {
  DCHECK(responses.empty() || responses.size() == accounts.size());

  std::vector<std::string> granted_accounts;
  for (size_t i = 0; i < responses.size(); i++) {
    if (responses[i] == CONTENT_SETTING_ALLOW) {
      granted_accounts.push_back(accounts[i]);
    }
  }

  // The responses array will be empty if operation failed.
  std::move(callback).Run(!responses.empty(), granted_accounts);
}

void OnGetAllowedAccounts(
    BraveWalletProviderDelegate::GetAllowedAccountsCallback callback,
    bool success,
    const std::vector<std::string>& allowed_accounts) {
  std::move(callback).Run(success, allowed_accounts);
}

}  // namespace

BraveWalletProviderDelegateImpl::BraveWalletProviderDelegateImpl(
    content::WebContents* web_contents,
    content::RenderFrameHost* const render_frame_host)
    : WebContentsObserver(web_contents),
      web_contents_(web_contents),
      host_id_(render_frame_host->GetGlobalId()),
      weak_ptr_factory_(this) {
  keyring_controller_ =
      brave_wallet::KeyringControllerFactory::GetControllerForContext(
          web_contents->GetBrowserContext());
  DCHECK(keyring_controller_);
}

BraveWalletProviderDelegateImpl::~BraveWalletProviderDelegateImpl() = default;

GURL BraveWalletProviderDelegateImpl::GetOrigin() const {
  auto* rfh = content::RenderFrameHost::FromID(host_id_);
  return rfh ? rfh->GetLastCommittedURL().GetOrigin() : GURL();
}

void BraveWalletProviderDelegateImpl::ShowPanel() {
  ::brave_wallet::ShowPanel(web_contents_);
}

void BraveWalletProviderDelegateImpl::RequestEthereumPermissions(
    RequestEthereumPermissionsCallback callback) {
  GetAllowedAccounts(base::BindOnce(
      &BraveWalletProviderDelegateImpl::ContinueRequestEthereumPermissions,
      weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveWalletProviderDelegateImpl::ContinueRequestEthereumPermissions(
    RequestEthereumPermissionsCallback callback,
    bool success,
    const std::vector<std::string>& allowed_accounts) {
  if (!success) {
    std::move(callback).Run(false, std::vector<std::string>());
    return;
  }

  if (success && !allowed_accounts.empty()) {
    std::move(callback).Run(true, allowed_accounts);
    return;
  }

  // Request accounts if no accounts are connected.
  keyring_controller_->GetDefaultKeyringInfo(base::BindOnce(
      &BraveWalletProviderDelegateImpl::
          ContinueRequestEthereumPermissionsKeyringInfo,
      weak_ptr_factory_.GetWeakPtr(), std::move(callback), allowed_accounts));
}

void BraveWalletProviderDelegateImpl::
    ContinueRequestEthereumPermissionsKeyringInfo(
        RequestEthereumPermissionsCallback callback,
        const std::vector<std::string>& allowed_accounts,
        brave_wallet::mojom::KeyringInfoPtr keyring_info) {
  if (!keyring_info->is_default_keyring_created) {
    ShowWalletOnboarding(web_contents_);
    std::move(callback).Run(false, std::vector<std::string>());
    return;
  }

  std::vector<std::string> addresses;
  for (const auto& account_info : keyring_info->account_infos) {
    addresses.push_back(account_info->address);
  }
  permissions::BraveEthereumPermissionContext::RequestPermissions(
      content::RenderFrameHost::FromID(host_id_), addresses,
      base::BindOnce(&OnRequestEthereumPermissions, addresses,
                     std::move(callback)));
}

void BraveWalletProviderDelegateImpl::GetAllowedAccounts(
    GetAllowedAccountsCallback callback) {
  keyring_controller_->GetSelectedAccount(base::BindOnce(
      &BraveWalletProviderDelegateImpl::ContinueGetAllowedAccounts,
      weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveWalletProviderDelegateImpl::ContinueGetAllowedAccounts(
    BraveWalletProviderDelegate::GetAllowedAccountsCallback callback,
    const absl::optional<std::string>& selected_account) {
  keyring_controller_->GetDefaultKeyringInfo(base::BindOnce(
      [](const content::GlobalRenderFrameHostId& host_id,
         GetAllowedAccountsCallback callback,
         const absl::optional<std::string>& selected_account,
         brave_wallet::mojom::KeyringInfoPtr keyring_info) {
        std::vector<std::string> addresses;
        if (!keyring_info->is_locked) {
          for (const auto& account_info : keyring_info->account_infos) {
            // If one of the selected accounts is an allowed account, then make
            // the selected account the first item that is returned.
            if (selected_account &&
                base::CompareCaseInsensitiveASCII(account_info->address,
                                                  *selected_account) == 0) {
              addresses.insert(addresses.begin(), account_info->address);
            } else {
              addresses.push_back(account_info->address);
            }
          }
        }
        permissions::BraveEthereumPermissionContext::GetAllowedAccounts(
            content::RenderFrameHost::FromID(host_id), addresses,
            base::BindOnce(&OnGetAllowedAccounts, std::move(callback)));
      },
      host_id_, std::move(callback), selected_account));
}

void BraveWalletProviderDelegateImpl::WebContentsDestroyed() {
  web_contents_ = nullptr;
}

}  // namespace brave_wallet
