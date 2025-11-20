// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_PAGE_HANDLER_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_PAGE_HANDLER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace web {
class WebState;
}  // namespace web

class WalletPageHandler : public brave_wallet::mojom::PageHandler {
 public:
  WalletPageHandler(
      web::WebState* web_state,
      mojo::PendingReceiver<brave_wallet::mojom::PageHandler> receiver);

  WalletPageHandler(const WalletPageHandler&) = delete;
  WalletPageHandler& operator=(const WalletPageHandler&) = delete;
  ~WalletPageHandler() override;

  // brave_wallet::mojom::PageHandler
  void ShowApprovePanelUI() override;
  void ShowWalletBackupUI() override;
  void UnlockWalletUI() override;
  void ShowOnboarding(bool is_new_wallet) override;

 private:
  raw_ptr<web::WebState> web_state_;
  mojo::Receiver<brave_wallet::mojom::PageHandler> receiver_;
  base::WeakPtrFactory<WalletPageHandler> weak_ptr_factory_{this};
};

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_PAGE_HANDLER_H_
