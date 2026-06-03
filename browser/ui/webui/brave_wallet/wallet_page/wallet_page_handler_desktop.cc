/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/notreached.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_page/wallet_page_handler.h"
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
  BraveWalletTabHelper::FromWebContents(
      webui_controller_->web_ui()->GetWebContents())
      ->ShowApproveWalletBubble();
}

void WalletPageHandler::ShowWalletBackupUI() {
  NOTREACHED();
}

void WalletPageHandler::UnlockWalletUI() {
  NOTREACHED();
}

void WalletPageHandler::ShowOnboarding(bool is_new_wallet) {
  // iOS/Mobile only
  NOTREACHED();
}

void WalletPageHandler::OpenWalletHome() {
  // iOS/Mobile only
  NOTREACHED();
}

}  // namespace brave_wallet
