// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_COMMON_HANDLER_WALLET_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_COMMON_HANDLER_WALLET_HANDLER_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace content {
class WebUI;
}

class WalletHandler : public brave_wallet::mojom::WalletHandler {
 public:
  WalletHandler(
      mojo::PendingReceiver<brave_wallet::mojom::WalletHandler> receiver,
      mojo::PendingRemote<brave_wallet::mojom::Page> page,
      content::WebUI* web_ui,
      ui::MojoWebUIController* webui_controller);

  WalletHandler(const WalletHandler&) = delete;
  WalletHandler& operator=(const WalletHandler&) = delete;
  ~WalletHandler() override;

  // brave_wallet::mojom::WalletHandler:
  void GetWalletInfo(GetWalletInfoCallback) override;
  void LockWallet() override;
  void UnlockWallet(const std::string& password, UnlockWalletCallback) override;
  void GetAssetPrice(const std::string& asset, GetAssetPriceCallback) override;
  void GetAssetPriceHistory(const std::string& asset,
                            brave_wallet::mojom::AssetPriceTimeframe timeframe,
                            GetAssetPriceHistoryCallback) override;
  void AddFavoriteApp(brave_wallet::mojom::AppItemPtr app_item) override;
  void RemoveFavoriteApp(brave_wallet::mojom::AppItemPtr app_item) override;
  void NotifyWalletBackupComplete() override;

 private:
  void OnGetPrice(GetAssetPriceCallback callback,
                  bool success,
                  const std::string& price);
  void OnGetPriceHistory(
      GetAssetPriceHistoryCallback callback,
      bool success,
      std::vector<brave_wallet::mojom::AssetTimePricePtr> values);

  // TODO(bbondy): This needs to be persisted in prefs
  std::vector<brave_wallet::mojom::AppItemPtr> favorite_apps;
  mojo::Receiver<brave_wallet::mojom::WalletHandler> receiver_;
  mojo::Remote<brave_wallet::mojom::Page> page_;
  content::WebUI* const web_ui_;

  base::WeakPtrFactory<WalletHandler> weak_ptr_factory_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_COMMON_HANDLER_WALLET_HANDLER_H_
