// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/brave_wallet/wallet_page_handler.h"

#include "brave/ios/web/webui/brave_webui_messaging_tab_helper.h"
#include "brave/ios/web/webui/brave_webui_messaging_tab_helper_delegate.h"
#include "ios/web/public/web_state.h"

namespace {
id<BraveWebUIMessagingTabHelperDelegate> GetNativeInterface(
    web::WebState* web_state) {
  if (auto* tab_helper =
          BraveWebUIMessagingTabHelper::FromWebState(web_state)) {
    if (id<BraveWebUIMessagingTabHelperDelegate> delegate =
            tab_helper->GetBridgingDelegate()) {
      return delegate;
    }
  }

  return nullptr;
}
}  // namespace

WalletPageHandler::WalletPageHandler(
    web::WebState* web_state,
    mojo::PendingReceiver<brave_wallet::mojom::PageHandler> receiver)
    : web_state_(web_state), receiver_(this, std::move(receiver)) {}

WalletPageHandler::~WalletPageHandler() = default;

void WalletPageHandler::ShowApprovePanelUI() {
  if (auto delegate = GetNativeInterface(web_state_)) {
    [delegate webUIShowWalletApprovePanelUI];
  }
}

void WalletPageHandler::ShowWalletBackupUI() {
  if (auto delegate = GetNativeInterface(web_state_)) {
    [delegate webUIShowWalletBackupUI];
  }
}

void WalletPageHandler::UnlockWalletUI() {
  if (auto delegate = GetNativeInterface(web_state_)) {
    [delegate webUIUnlockWallet];
  }
}
