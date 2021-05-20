// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_wallet/common_handler/wallet_handler.h"

#include <utility>

#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui.h"

namespace {

BraveWalletService* GetBraveWalletService(content::BrowserContext* context) {
  return BraveWalletServiceFactory::GetInstance()->GetForContext(context);
}

}  // namespace

WalletHandler::WalletHandler(
    mojo::PendingReceiver<wallet_ui::mojom::WalletHandler> receiver,
    mojo::PendingRemote<wallet_ui::mojom::Page> page,
    content::WebUI* web_ui,
    ui::MojoWebUIController* webui_controller)
    : receiver_(this, std::move(receiver)),
      page_(std::move(page)),
      web_ui_(web_ui) {}

WalletHandler::~WalletHandler() = default;

void WalletHandler::GetWalletInfo(GetWalletInfoCallback callback) {
  auto* profile = Profile::FromWebUI(web_ui_);
  auto* keyring_controller =
      GetBraveWalletService(profile)->keyring_controller();
  std::move(callback).Run(keyring_controller->IsDefaultKeyringCreated(),
                          keyring_controller->IsLocked());
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
