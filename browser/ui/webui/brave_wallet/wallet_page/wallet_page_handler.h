/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_PAGE_WALLET_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_PAGE_WALLET_PAGE_HANDLER_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace content {
class WebUIController;
}
class Profile;

namespace brave_wallet {

class WalletPageHandler : public mojom::PageHandler {
 public:
  WalletPageHandler(mojo::PendingReceiver<mojom::PageHandler> receiver,
                    Profile* profile,
                    content::WebUIController& webui_controller);

  WalletPageHandler(const WalletPageHandler&) = delete;
  WalletPageHandler& operator=(const WalletPageHandler&) = delete;
  ~WalletPageHandler() override;

  void ShowApprovePanelUI() override;
  void ShowWalletBackupUI() override;
  void UnlockWalletUI() override;
  void ShowOnboarding(bool is_new_wallet) override;
  void OpenWalletHome() override;

 private:
  raw_ptr<Profile> profile_ = nullptr;  // NOT OWNED
  mojo::Receiver<mojom::PageHandler> receiver_;
  const raw_ref<content::WebUIController> webui_controller_;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_PAGE_WALLET_PAGE_HANDLER_H_
