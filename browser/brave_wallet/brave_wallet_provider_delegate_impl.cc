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
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "brave/components/permissions/contexts/brave_ethereum_permission_context.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

// Common logic for filtering the list of accounts based on the selected account
std::vector<std::string> FilterAccounts(
    const std::vector<std::string>& accounts,
    const absl::optional<std::string>& selected_account) {
  // If one of the accounts matches the selected account, then only
  // return that account.  This is for webcompat reasons.
  // Some Dapps select the first account in the list, and some the
  // last. So having only 1 item returned here makes it work for
  // all Dapps.
  std::vector<std::string> filtered_accounts;
  for (const auto& account : accounts) {
    if (selected_account &&
        base::CompareCaseInsensitiveASCII(account, *selected_account) == 0) {
      filtered_accounts.clear();
      filtered_accounts.push_back(account);
      break;
    } else {
      filtered_accounts.push_back(account);
    }
  }
  return filtered_accounts;
}

void OnRequestEthereumPermissions(
    const std::vector<std::string>& accounts,
    const absl::optional<std::string>& selected_account,
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
  bool success = !responses.empty();
  std::move(callback).Run(
      FilterAccounts(granted_accounts, selected_account),
      success ? mojom::ProviderError::kSuccess
              : mojom::ProviderError::kInternalError,
      success ? "" : l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

void OnGetAllowedAccounts(
    bool include_accounts_when_locked,
    const absl::optional<std::string>& selected_account,
    bool keyring_locked,
    BraveWalletProviderDelegate::GetAllowedAccountsCallback callback,
    bool success,
    const std::vector<std::string>& allowed_accounts) {
  std::vector<std::string> filtered_accounts;
  if (!keyring_locked || include_accounts_when_locked) {
    filtered_accounts = FilterAccounts(allowed_accounts, selected_account);
  }

  std::move(callback).Run(
      filtered_accounts,
      success ? mojom::ProviderError::kSuccess
              : mojom::ProviderError::kInternalError,
      success ? "" : l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
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
  return rfh ? rfh->GetLastCommittedOrigin().GetURL() : GURL();
}

void BraveWalletProviderDelegateImpl::ShowPanel() {
  ::brave_wallet::ShowPanel(web_contents_);
}

void BraveWalletProviderDelegateImpl::RequestEthereumPermissions(
    RequestEthereumPermissionsCallback callback) {
  // Check if there's already a permission request in progress
  auto* rfh = content::RenderFrameHost::FromID(host_id_);
  if (rfh &&
      permissions::BraveEthereumPermissionContext::HasRequestsInProgress(rfh)) {
    std::move(callback).Run(
        std::vector<std::string>(), mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
    return;
  }

  GetAllowedAccounts(
      false,
      base::BindOnce(
          &BraveWalletProviderDelegateImpl::ContinueRequestEthereumPermissions,
          weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveWalletProviderDelegateImpl::ContinueRequestEthereumPermissions(
    RequestEthereumPermissionsCallback callback,
    const std::vector<std::string>& allowed_accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess) {
    std::move(callback).Run(std::vector<std::string>(), error, error_message);
    return;
  }

  if (error == mojom::ProviderError::kSuccess && !allowed_accounts.empty()) {
    std::move(callback).Run(allowed_accounts, mojom::ProviderError::kSuccess,
                            "");
    return;
  }

  // Request accounts if no accounts are connected.
  keyring_controller_->GetKeyringInfo(
      brave_wallet::kDefaultKeyringId,
      base::BindOnce(&BraveWalletProviderDelegateImpl::
                         ContinueRequestEthereumPermissionsKeyringInfo,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveWalletProviderDelegateImpl::
    ContinueRequestEthereumPermissionsKeyringInfo(
        RequestEthereumPermissionsCallback callback,
        brave_wallet::mojom::KeyringInfoPtr keyring_info) {
  if (!keyring_info->is_default_keyring_created) {
    ShowWalletOnboarding(web_contents_);
    std::move(callback).Run(
        std::vector<std::string>(), mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::vector<std::string> addresses;
  for (const auto& account_info : keyring_info->account_infos) {
    addresses.push_back(account_info->address);
  }

  if (keyring_info->is_locked) {
    std::move(callback).Run(std::vector<std::string>(),
                            mojom::ProviderError::kSuccess, "");
    return;
  }

  permissions::BraveEthereumPermissionContext::RequestPermissions(
      content::RenderFrameHost::FromID(host_id_), addresses,
      base::BindOnce(&OnRequestEthereumPermissions, addresses,
                     keyring_controller_->GetSelectedAccount(),
                     std::move(callback)));
}

void BraveWalletProviderDelegateImpl::GetAllowedAccounts(
    bool include_accounts_when_locked,
    GetAllowedAccountsCallback callback) {
  absl::optional<std::string> selected_account =
      keyring_controller_->GetSelectedAccount();
  keyring_controller_->GetKeyringInfo(
      brave_wallet::kDefaultKeyringId,
      base::BindOnce(
          [](const content::GlobalRenderFrameHostId& host_id,
             GetAllowedAccountsCallback callback,
             bool include_accounts_when_locked,
             const absl::optional<std::string>& selected_account,
             brave_wallet::mojom::KeyringInfoPtr keyring_info) {
            std::vector<std::string> addresses;
            for (const auto& account_info : keyring_info->account_infos) {
              addresses.push_back(account_info->address);
            }

            permissions::BraveEthereumPermissionContext::GetAllowedAccounts(
                content::RenderFrameHost::FromID(host_id), addresses,
                base::BindOnce(&OnGetAllowedAccounts,
                               include_accounts_when_locked, selected_account,
                               keyring_info->is_locked, std::move(callback)));
          },
          host_id_, std::move(callback), include_accounts_when_locked,
          selected_account));
}

void BraveWalletProviderDelegateImpl::WebContentsDestroyed() {
  web_contents_ = nullptr;
}

}  // namespace brave_wallet
