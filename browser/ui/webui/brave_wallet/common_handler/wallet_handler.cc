// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file, //
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_wallet/common_handler/wallet_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/asset_ratio_controller.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui.h"

namespace {

brave_wallet::BraveWalletService* GetBraveWalletService(
    content::BrowserContext* context) {
  return brave_wallet::BraveWalletServiceFactory::GetInstance()->GetForContext(
      context);
}

}  // namespace

WalletHandler::WalletHandler(
    mojo::PendingReceiver<brave_wallet::mojom::WalletHandler> receiver,
    mojo::PendingRemote<brave_wallet::mojom::Page> page,
    content::WebUI* web_ui,
    ui::MojoWebUIController* webui_controller)
    : receiver_(this, std::move(receiver)),
      page_(std::move(page)),
      web_ui_(web_ui) {}

WalletHandler::~WalletHandler() = default;

void WalletHandler::GetWalletInfo(GetWalletInfoCallback callback) {
  auto* profile = Profile::FromWebUI(web_ui_);
  std::vector<std::string> accounts;
  auto* service = GetBraveWalletService(profile);
  auto* keyring_controller = service->keyring_controller();
  auto* default_keyring = keyring_controller->GetDefaultKeyring();
  if (default_keyring) {
    accounts = default_keyring->GetAccounts();
  }

  std::vector<brave_wallet::mojom::AppItemPtr> favorite_apps_copy(
      favorite_apps.size());
  std::transform(
      favorite_apps.begin(), favorite_apps.end(), favorite_apps_copy.begin(),
      [](const brave_wallet::mojom::AppItemPtr& favorite_app)
          -> brave_wallet::mojom::AppItemPtr { return favorite_app.Clone(); });
  std::move(callback).Run(keyring_controller->IsDefaultKeyringCreated(),
                          keyring_controller->IsLocked(),
                          std::move(favorite_apps_copy),
                          service->IsWalletBackedUp(), accounts);
}

void WalletHandler::LockWallet() {
  auto* profile = Profile::FromWebUI(web_ui_);
  auto* keyring_controller =
      GetBraveWalletService(profile)->keyring_controller();
  keyring_controller->Lock();
}

void WalletHandler::UnlockWallet(const std::string& password,
                                 UnlockWalletCallback callback) {
  auto* profile = Profile::FromWebUI(web_ui_);
  auto* keyring_controller =
      GetBraveWalletService(profile)->keyring_controller();
  bool result = keyring_controller->Unlock(password);
  std::move(callback).Run(result);
}

void WalletHandler::GetAssetPrice(const std::string& asset,
                                  GetAssetPriceCallback callback) {
  auto* profile = Profile::FromWebUI(web_ui_);
  auto* asset_ratio_controller =
      GetBraveWalletService(profile)->asset_ratio_controller();
  asset_ratio_controller->GetPrice(asset,
      base::BindOnce(&WalletHandler::OnGetPrice,
                     base::Unretained(this), std::move(callback)));
}

void WalletHandler::OnGetPrice(GetAssetPriceCallback callback, bool success, const std::string& price) {
  std::move(callback).Run(price);
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

void WalletHandler::NotifyWalletBackupComplete() {
  auto* profile = Profile::FromWebUI(web_ui_);
  auto* service = GetBraveWalletService(profile);
  service->NotifyWalletBackupComplete();
}
