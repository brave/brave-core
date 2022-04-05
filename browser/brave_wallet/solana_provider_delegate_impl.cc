/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/solana_provider_delegate_impl.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_helper.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

bool IsAccountAllowed(const std::vector<std::string>& allowed_accounts,
                      const std::string& account) {
  for (const auto& allowed_account : allowed_accounts) {
    if (base::CompareCaseInsensitiveASCII(account, allowed_account) == 0)
      return true;
  }
  return false;
}

void OnRequestSolanaPermission(
    const std::vector<std::string>& accounts,
    const std::string& selected_account,
    SolanaProviderDelegate::RequestSolanaPermissionCallback callback,
    const std::vector<ContentSetting>& responses) {
  DCHECK(responses.empty() || responses.size() == accounts.size());

  std::vector<std::string> granted_accounts;
  for (size_t i = 0; i < responses.size(); i++) {
    if (responses[i] == CONTENT_SETTING_ALLOW) {
      granted_accounts.push_back(accounts[i]);
    }
  }

  // The responses array will be empty if operation failed.
  bool success = !responses.empty() &&
                 IsAccountAllowed(granted_accounts, selected_account);
  std::move(callback).Run(
      selected_account,
      success ? mojom::SolanaProviderError::kSuccess
              : mojom::SolanaProviderError::kInternalError,
      success ? "" : l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

void OnIsSelectedAccountAllowed(
    const std::string& selected_account,
    SolanaProviderDelegate::IsSelectedAccountAllowedCallback callback,
    bool success,
    const std::vector<std::string>& allowed_accounts) {
  bool allowed = IsAccountAllowed(allowed_accounts, selected_account);

  std::move(callback).Run(selected_account, allowed);
}

}  // namespace

SolanaProviderDelegateImpl::SolanaProviderDelegateImpl(
    content::WebContents* web_contents,
    content::RenderFrameHost* const render_frame_host)
    : WebContentsObserver(web_contents),
      web_contents_(web_contents),
      host_id_(render_frame_host->GetGlobalId()),
      weak_ptr_factory_(this) {
  if (!keyring_service_) {
    auto pending =
        KeyringServiceFactory::GetForContext(web_contents->GetBrowserContext());
    keyring_service_.Bind(std::move(pending));
  }
  DCHECK(keyring_service_);
}

SolanaProviderDelegateImpl::~SolanaProviderDelegateImpl() = default;

GURL SolanaProviderDelegateImpl::GetOrigin() const {
  auto* rfh = content::RenderFrameHost::FromID(host_id_);
  return rfh ? rfh->GetLastCommittedOrigin().GetURL() : GURL();
}

void SolanaProviderDelegateImpl::ShowPanel() {
  ::brave_wallet::ShowPanel(web_contents_);
}

void SolanaProviderDelegateImpl::RequestSolanaPermission(
    RequestSolanaPermissionCallback callback) {
  // Check if there's already a permission request in progress
  auto* rfh = content::RenderFrameHost::FromID(host_id_);
  if (rfh && permissions::BraveWalletPermissionContext::HasRequestsInProgress(
                 rfh, permissions::RequestType::kBraveSolana)) {
    std::move(callback).Run(
        "", mojom::SolanaProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
    return;
  }

  IsSelectedAccountAllowed(base::BindOnce(
      &SolanaProviderDelegateImpl::ContinueRequestSolanaPermission,
      weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void SolanaProviderDelegateImpl::IsSelectedAccountAllowed(
    IsSelectedAccountAllowedCallback callback) {
  keyring_service_->GetSelectedAccount(
      mojom::CoinType::SOL,
      base::BindOnce(
          &SolanaProviderDelegateImpl::ContinueIsSelectedAccountAllowed,
          weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void SolanaProviderDelegateImpl::ContinueRequestSolanaPermission(
    RequestSolanaPermissionCallback callback,
    const absl::optional<std::string>& selected_account,
    bool is_selected_account_allowed) {
  if (!selected_account) {
    std::move(callback).Run(
        "", mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  if (is_selected_account_allowed) {
    std::move(callback).Run(*selected_account,
                            mojom::SolanaProviderError::kSuccess, "");
  } else {
    std::vector<std::string> addresses;
    addresses.push_back(*selected_account);
    permissions::BraveWalletPermissionContext::RequestPermissions(
        ContentSettingsType::BRAVE_SOLANA,
        content::RenderFrameHost::FromID(host_id_), addresses,
        base::BindOnce(&OnRequestSolanaPermission, addresses, *selected_account,
                       std::move(callback)));
  }
}

void SolanaProviderDelegateImpl::ContinueIsSelectedAccountAllowed(
    IsSelectedAccountAllowedCallback callback,
    const absl::optional<std::string>& selected_account) {
  if (!selected_account.has_value()) {
    std::move(callback).Run(absl::nullopt, false);
    return;
  }
  std::vector<std::string> addresses;
  addresses.push_back(*selected_account);
  permissions::BraveWalletPermissionContext::GetAllowedAccounts(
      ContentSettingsType::BRAVE_SOLANA,
      content::RenderFrameHost::FromID(host_id_), addresses,
      base::BindOnce(&OnIsSelectedAccountAllowed, *selected_account,
                     std::move(callback)));
}

void SolanaProviderDelegateImpl::WebContentsDestroyed() {
  web_contents_ = nullptr;
}

}  // namespace brave_wallet
