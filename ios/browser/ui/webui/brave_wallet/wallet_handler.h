// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_HANDLER_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_HANDLER_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

class ProfileIOS;

namespace brave_wallet {

class BraveWalletService;

class WalletHandler : public mojom::WalletHandler {
 public:
  WalletHandler(mojo::PendingReceiver<mojom::WalletHandler> receiver,
                ProfileIOS* browser_state);

  WalletHandler(const WalletHandler&) = delete;
  WalletHandler& operator=(const WalletHandler&) = delete;
  ~WalletHandler() override;

  // mojom::WalletHandler:
  void GetWalletInfo(GetWalletInfoCallback) override;

 private:
  mojo::Receiver<mojom::WalletHandler> receiver_{this};
  const raw_ptr<BraveWalletService> brave_wallet_service_ = nullptr;
};

}  // namespace brave_wallet

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_HANDLER_H_
