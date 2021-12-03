// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_COMMON_HANDLER_WALLET_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_COMMON_HANDLER_WALLET_HANDLER_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class Profile;

class WalletHandler : public brave_wallet::mojom::WalletHandler {
 public:
  WalletHandler(
      mojo::PendingReceiver<brave_wallet::mojom::WalletHandler> receiver,
      Profile* profile);

  WalletHandler(const WalletHandler&) = delete;
  WalletHandler& operator=(const WalletHandler&) = delete;
  ~WalletHandler() override;

  // brave_wallet::mojom::WalletHandler:
  void GetWalletInfo(GetWalletInfoCallback) override;

  void AddFavoriteApp(brave_wallet::mojom::AppItemPtr app_item) override;
  void RemoveFavoriteApp(brave_wallet::mojom::AppItemPtr app_item) override;

 private:
  void EnsureConnected();
  void OnConnectionError();

  void OnGetWalletInfo(
      GetWalletInfoCallback callback,
      std::vector<brave_wallet::mojom::KeyringInfoPtr> keyring_info);

  mojo::Remote<brave_wallet::mojom::KeyringService> keyring_service_;

  // TODO(bbondy): This needs to be persisted in prefs
  std::vector<brave_wallet::mojom::AppItemPtr> favorite_apps;
  mojo::Receiver<brave_wallet::mojom::WalletHandler> receiver_;

  raw_ptr<Profile> profile_ = nullptr;  // NOT OWNED
  base::WeakPtrFactory<WalletHandler> weak_ptr_factory_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_COMMON_HANDLER_WALLET_HANDLER_H_
