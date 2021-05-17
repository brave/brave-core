// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_wallet/page_handler/wallet_panel_page_handler.h"

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

WalletPanelPageHandler::WalletPanelPageHandler(
    mojo::PendingReceiver<wallet_panel::mojom::PageHandler> receiver,
    mojo::PendingRemote<wallet_panel::mojom::Page> page,
    content::WebUI* web_ui,
    ui::MojoBubbleWebUIController* webui_controller)
    : receiver_(this, std::move(receiver)),
      page_(std::move(page)),
      web_ui_(web_ui),
      webui_controller_(webui_controller) {
  Observe(web_ui_->GetWebContents());
}

WalletPanelPageHandler::~WalletPanelPageHandler() = default;

void WalletPanelPageHandler::ShowUI() {
  auto embedder = webui_controller_->embedder();
  if (embedder) {
    embedder->ShowUI();
  }
}

void WalletPanelPageHandler::CloseUI() {
  auto embedder = webui_controller_->embedder();
  if (embedder) {
    embedder->CloseUI();
  }
}

void WalletPanelPageHandler::CreateWallet(const std::string& password,
                                          CreateWalletCallback callback) {
  auto* profile = Profile::FromWebUI(web_ui_);
  auto* keyring_controller =
      GetBraveWalletService(profile)->keyring_controller();
  keyring_controller->CreateDefaultKeyring(password);
  std::move(callback).Run(keyring_controller->GetMnemonicForDefaultKeyring());
}

void WalletPanelPageHandler::OnVisibilityChanged(
    content::Visibility visibility) {
  webui_hidden_ = visibility == content::Visibility::HIDDEN;
}
