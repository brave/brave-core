// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_COMMON_HANDLER_WALLET_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_COMMON_HANDLER_WALLET_HANDLER_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet_ui/wallet_ui.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace content {
class WebUI;
}

class WalletHandler : public wallet_ui::mojom::WalletHandler {
 public:
  WalletHandler(mojo::PendingReceiver<wallet_ui::mojom::WalletHandler> receiver,
                mojo::PendingRemote<wallet_ui::mojom::Page> page,
                content::WebUI* web_ui,
                ui::MojoWebUIController* webui_controller);

  WalletHandler(const WalletHandler&) = delete;
  WalletHandler& operator=(const WalletHandler&) = delete;
  ~WalletHandler() override;

  // wallet_ui::mojom::WalletHandler:
  void GetWalletInfo(GetWalletInfoCallback) override;
  void LockWallet() override;
  void UnlockWallet(const std::string& password, UnlockWalletCallback) override;
  void AddFavoriteApp(wallet_ui::mojom::AppItemPtr app_item) override;
  void RemoveFavoriteApp(wallet_ui::mojom::AppItemPtr app_item) override;
  void NotifyWalletBackupComplete() override;

 private:
  // TODO(bbondy): This needs to be persisted in prefs
  std::vector<wallet_ui::mojom::AppItemPtr> favorite_apps;
  mojo::Receiver<wallet_ui::mojom::WalletHandler> receiver_;
  mojo::Remote<wallet_ui::mojom::Page> page_;
  content::WebUI* const web_ui_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_COMMON_HANDLER_WALLET_HANDLER_H_
