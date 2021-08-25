// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file, //
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_wallet/common_handler/wallet_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "brave/browser/brave_wallet/keyring_controller_factory.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "chrome/browser/profiles/profile.h"

WalletHandler::WalletHandler(
    mojo::PendingReceiver<brave_wallet::mojom::WalletHandler> receiver,
    Profile* profile)
    : receiver_(this, std::move(receiver)),
      profile_(profile),
      weak_ptr_factory_(this) {}

WalletHandler::~WalletHandler() = default;

void WalletHandler::EnsureConnected() {
  if (!keyring_controller_) {
    auto pending =
        brave_wallet::KeyringControllerFactory::GetInstance()->GetForContext(
            profile_);
    keyring_controller_.Bind(std::move(pending));
  }
  DCHECK(keyring_controller_);
  keyring_controller_.set_disconnect_handler(base::BindOnce(
      &WalletHandler::OnConnectionError, weak_ptr_factory_.GetWeakPtr()));
}

void WalletHandler::OnConnectionError() {
  keyring_controller_.reset();
  EnsureConnected();
}

void WalletHandler::GetWalletInfo(GetWalletInfoCallback callback) {
  EnsureConnected();

  keyring_controller_->GetDefaultKeyringInfo(
      base::BindOnce(&WalletHandler::OnGetWalletInfo,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void WalletHandler::OnGetWalletInfo(
    GetWalletInfoCallback callback,
    brave_wallet::mojom::KeyringInfoPtr keyring_info) {
  std::vector<brave_wallet::mojom::AppItemPtr> favorite_apps_copy(
      favorite_apps.size());
  std::transform(
      favorite_apps.begin(), favorite_apps.end(), favorite_apps_copy.begin(),
      [](const brave_wallet::mojom::AppItemPtr& favorite_app)
          -> brave_wallet::mojom::AppItemPtr { return favorite_app.Clone(); });
  std::move(callback).Run(
      keyring_info->is_default_keyring_created, keyring_info->is_locked,
      std::move(favorite_apps_copy), keyring_info->is_backed_up,
      std::move(keyring_info->account_infos));
}

void WalletHandler::AddFavoriteApp(
    const brave_wallet::mojom::AppItemPtr app_item) {
  favorite_apps.push_back(app_item->Clone());
}

void WalletHandler::RemoveFavoriteApp(
    brave_wallet::mojom::AppItemPtr app_item) {
  favorite_apps.erase(
      remove_if(favorite_apps.begin(), favorite_apps.end(),
                [&app_item](const brave_wallet::mojom::AppItemPtr& it) -> bool {
                  return it->name == app_item->name;
                }));
}
