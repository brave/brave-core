// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file, //
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_wallet/common_handler/wallet_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/browser/profiles/profile.h"

WalletHandler::WalletHandler(
    mojo::PendingReceiver<brave_wallet::mojom::WalletHandler> receiver,
    Profile* profile)
    : receiver_(this, std::move(receiver)),
      profile_(profile),
      weak_ptr_factory_(this) {}

WalletHandler::~WalletHandler() = default;

void WalletHandler::EnsureConnected() {
  if (!keyring_service_) {
    auto pending =
        brave_wallet::KeyringServiceFactory::GetInstance()->GetForContext(
            profile_);
    keyring_service_.Bind(std::move(pending));
  }
  DCHECK(keyring_service_);
  keyring_service_.set_disconnect_handler(base::BindOnce(
      &WalletHandler::OnConnectionError, weak_ptr_factory_.GetWeakPtr()));
}

void WalletHandler::OnConnectionError() {
  keyring_service_.reset();
  EnsureConnected();
}

void WalletHandler::GetWalletInfo(GetWalletInfoCallback callback) {
  EnsureConnected();
  std::vector<std::string> ids(1, brave_wallet::mojom::kDefaultKeyringId);
  if (brave_wallet::IsFilecoinEnabled()) {
    ids.push_back(brave_wallet::mojom::kFilecoinKeyringId);
  }
  keyring_service_->GetKeyringsInfo(
      ids, base::BindOnce(&WalletHandler::OnGetWalletInfo,
                          weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void WalletHandler::OnGetWalletInfo(
    GetWalletInfoCallback callback,
    std::vector<brave_wallet::mojom::KeyringInfoPtr> keyring_infos) {
  std::vector<brave_wallet::mojom::AppItemPtr> favorite_apps_copy(
      favorite_apps.size());
  std::transform(
      favorite_apps.begin(), favorite_apps.end(), favorite_apps_copy.begin(),
      [](const brave_wallet::mojom::AppItemPtr& favorite_app)
          -> brave_wallet::mojom::AppItemPtr { return favorite_app.Clone(); });
  DCHECK(keyring_infos.size()) << "Default keyring must be returned";
  std::vector<brave_wallet::mojom::AccountInfoPtr> account_infos;
  for (const auto& keyring_info : keyring_infos) {
    account_infos.insert(
        account_infos.end(),
        std::make_move_iterator(keyring_info->account_infos.begin()),
        std::make_move_iterator(keyring_info->account_infos.end()));
  }
  const auto& default_keyring = keyring_infos.front();
  DCHECK_EQ(default_keyring->id, brave_wallet::mojom::kDefaultKeyringId);
  std::move(callback).Run(
      default_keyring->is_default_keyring_created, default_keyring->is_locked,
      std::move(favorite_apps_copy), default_keyring->is_backed_up,
      std::move(account_infos), brave_wallet::IsFilecoinEnabled());
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
