// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_wallet/page_handler/wallet_page_handler.h"

#include <utility>

#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui.h"

#include "ui/webui/mojo_bubble_web_ui_controller.h"

namespace {

BraveWalletService* GetBraveWalletService(content::BrowserContext* context) {
  return BraveWalletServiceFactory::GetInstance()->GetForContext(context);
}

}  // namespace

WalletPageHandler::WalletPageHandler(
    mojo::PendingReceiver<wallet_ui::mojom::PageHandler> receiver,
    mojo::PendingRemote<wallet_ui::mojom::Page> page,
    content::WebUI* web_ui,
    ui::MojoWebUIController* webui_controller)
    : receiver_(this, std::move(receiver)),
      page_(std::move(page)),
      web_ui_(web_ui) {
  Observe(web_ui_->GetWebContents());
}

WalletPageHandler::~WalletPageHandler() = default;

void WalletPageHandler::CreateWallet(const std::string& password,
                                     CreateWalletCallback callback) {
  auto* profile = Profile::FromWebUI(web_ui_);
  auto* keyring_controller =
      GetBraveWalletService(profile)->keyring_controller();
  keyring_controller->CreateDefaultKeyring(password);
  std::move(callback).Run(keyring_controller->GetMnemonicForDefaultKeyring());
}

void WalletPageHandler::OnVisibilityChanged(content::Visibility visibility) {
  webui_hidden_ = visibility == content::Visibility::HIDDEN;
}
