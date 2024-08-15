// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_PAGE_HANDLER_WALLET_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_PAGE_HANDLER_WALLET_PAGE_HANDLER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class Profile;

class WalletPageHandler : public brave_wallet::mojom::PageHandler {
 public:
  WalletPageHandler(
      mojo::PendingReceiver<brave_wallet::mojom::PageHandler> receiver,
      Profile* profile);

  WalletPageHandler(const WalletPageHandler&) = delete;
  WalletPageHandler& operator=(const WalletPageHandler&) = delete;
  ~WalletPageHandler() override;

  void ShowApprovePanelUI() override;
  void ShowWalletBackupUI() override;
  void UnlockWalletUI() override;

 private:
  raw_ptr<Profile> profile_ = nullptr;  // NOT OWNED
  mojo::Receiver<brave_wallet::mojom::PageHandler> receiver_;
  base::WeakPtrFactory<WalletPageHandler> weak_ptr_factory_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_PAGE_HANDLER_WALLET_PAGE_HANDLER_H_
