/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "brave/browser/brave_wallet/keyring_controller_factory.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "brave/components/permissions/contexts/brave_ethereum_permission_context.h"
#include "components/content_settings/core/common/content_settings.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/remote.h"

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

  std::move(callback).Run(granted_accounts);
}

}  // namespace

BraveWalletProviderDelegateImpl::BraveWalletProviderDelegateImpl(
    content::WebContents* web_contents,
    content::RenderFrameHost* const render_frame_host)
    : web_contents_(web_contents),
      host_id_(render_frame_host->GetGlobalId()),
      weak_ptr_factory_(this) {}

BraveWalletProviderDelegateImpl::~BraveWalletProviderDelegateImpl() = default;

void BraveWalletProviderDelegateImpl::EnsureConnected() {
  if (!keyring_controller_) {
    auto pending =
        brave_wallet::KeyringControllerFactory::GetInstance()->GetForContext(
            web_contents_->GetBrowserContext());
    keyring_controller_.Bind(std::move(pending));
  }
  DCHECK(keyring_controller_);
  keyring_controller_.set_disconnect_handler(
      base::BindOnce(&BraveWalletProviderDelegateImpl::OnConnectionError,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BraveWalletProviderDelegateImpl::OnConnectionError() {
  keyring_controller_.reset();
  EnsureConnected();
}

void BraveWalletProviderDelegateImpl::RequestEthereumPermissions(
    RequestEthereumPermissionsCallback callback) {
  EnsureConnected();
  keyring_controller_->GetDefaultKeyringInfo(base::BindOnce(
      [](content::RenderFrameHost* rfh,
         RequestEthereumPermissionsCallback callback,
         brave_wallet::mojom::KeyringInfoPtr keyring_info) {
        std::vector<std::string> addresses;
        for (const auto& account_info : keyring_info->account_infos) {
          addresses.push_back(account_info->address);
        }
        permissions::BraveEthereumPermissionContext::RequestPermissions(
            rfh, addresses,
            base::BindOnce(&OnRequestEthereumPermissions, addresses,
                           std::move(callback)));
      },
      content::RenderFrameHost::FromID(host_id_), std::move(callback)));
}

}  // namespace brave_wallet
