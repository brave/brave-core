// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/brave_wallet/wallet_page_handler.h"

#include "base/notimplemented.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_wallet/ios/browser/wallet_page_handler_bridge.h"
#include "brave/ios/browser/ui/webui/brave_wallet/wallet_page_handler_bridge_holder.h"
#include "ios/web/public/web_state.h"

WalletPageHandler::WalletPageHandler(
    web::WebState* web_state,
    mojo::PendingReceiver<brave_wallet::mojom::PageHandler> receiver)
    : web_state_(web_state), receiver_(this, std::move(receiver)) {}

WalletPageHandler::~WalletPageHandler() = default;

void WalletPageHandler::ShowApprovePanelUI() {
  id<WalletPageHandlerBridge> bridge =
      brave_wallet::PageHandlerBridgeHolder::FromWebState(web_state_)->bridge();
  [bridge showApprovePanelUI];
}

void WalletPageHandler::ShowWalletBackupUI() {
  id<WalletPageHandlerBridge> bridge =
      brave_wallet::PageHandlerBridgeHolder::FromWebState(web_state_)->bridge();
  [bridge showWalletBackupUI];
}

void WalletPageHandler::UnlockWalletUI() {
  id<WalletPageHandlerBridge> bridge =
      brave_wallet::PageHandlerBridgeHolder::FromWebState(web_state_)->bridge();
  [bridge unlockWalletUI];
}

void WalletPageHandler::ShowOnboarding(bool is_new_wallet) {
  id<WalletPageHandlerBridge> bridge =
      brave_wallet::PageHandlerBridgeHolder::FromWebState(web_state_)->bridge();
  [bridge showOnboarding:is_new_wallet];
}

void WalletPageHandler::OpenWalletHome() {
  id<WalletPageHandlerBridge> bridge =
      brave_wallet::PageHandlerBridgeHolder::FromWebState(web_state_)->bridge();
  [bridge openWalletHome];
}

void WalletPageHandler::ScanQRCode(ScanQRCodeCallback callback) {
  id<WalletPageHandlerBridge> bridge =
      brave_wallet::PageHandlerBridgeHolder::FromWebState(web_state_)->bridge();
  auto shared_callback =
      std::make_shared<ScanQRCodeCallback>(std::move(callback));
  [bridge scanAddressQRCode:^(NSString* address) {
    std::move(*shared_callback).Run(base::SysNSStringToUTF8(address));
  }];
}
