// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_wallet/wallet_page/wallet_page_handler.h"

#include <utility>

#include "base/notreached.h"
#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_helper.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"

namespace brave_wallet {

WalletPageHandler::WalletPageHandler(
    mojo::PendingReceiver<brave_wallet::mojom::PageHandler> receiver,
    Profile* profile,
    content::WebUIController& webui_controller)
    : profile_(profile),
      receiver_(this, std::move(receiver)),
      webui_controller_(webui_controller) {}
WalletPageHandler::~WalletPageHandler() = default;

void WalletPageHandler::ShowApprovePanelUI() {
  auto* wc = webui_controller_->web_ui()->GetWebContents();
  if (!wc) {
    return;
  }

  ::brave_wallet::ShowPanel(wc);
}

void WalletPageHandler::ShowWalletBackupUI() {
  ::brave_wallet::ShowWalletBackup();
}

void WalletPageHandler::UnlockWalletUI() {
  ::brave_wallet::UnlockWallet();
}

void WalletPageHandler::ShowOnboarding(bool is_new_wallet) {
  // Android does not render wallet landing page.
  // Therefore, users won't be able to start onboarding from WebUI.
  NOTREACHED();
}

void WalletPageHandler::OpenWalletHome() {
  // This is only used by iOS behind the `isIOS` check.
  NOTREACHED();
}

}  // namespace brave_wallet
