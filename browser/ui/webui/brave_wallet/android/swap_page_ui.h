// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_ANDROID_SWAP_PAGE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_ANDROID_SWAP_PAGE_UI_H_

#include <memory>
#include <string>

#include "brave/browser/ui/webui/brave_wallet/common_handler/wallet_handler.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "content/public/browser/web_ui_controller.h"
#include "ui/webui/mojo_web_ui_controller.h"

class SwapPageHandler;
class SwapPageUI : public ui::MojoWebUIController,
                   public brave_wallet::mojom::PageHandlerFactory {
 public:
  explicit SwapPageUI(content::WebUI* web_ui, const std::string& name);
  SwapPageUI(const SwapPageUI&) = delete;
  SwapPageUI& operator=(const SwapPageUI&) = delete;
  ~SwapPageUI() override;

  // Instantiates the implementor of the mojom::PageHandlerFactory mojo
  // interface passing the pending receiver that will be internally bound.
  void BindInterface(
      mojo::PendingReceiver<brave_wallet::mojom::PageHandlerFactory> receiver);

 private:
  // brave_wallet::mojom::PageHandlerFactory:
  void CreatePageHandler(
      mojo::PendingRemote<brave_wallet::mojom::Page> page,
      mojo::PendingReceiver<brave_wallet::mojom::PageHandler> page_receiver,
      mojo::PendingReceiver<brave_wallet::mojom::WalletHandler> wallet_receiver,
      mojo::PendingReceiver<brave_wallet::mojom::JsonRpcService>
          json_rpc_service,
      mojo::PendingReceiver<brave_wallet::mojom::BitcoinWalletService>
          bitcoin_rpc_service_receiver,
      mojo::PendingReceiver<brave_wallet::mojom::SwapService> swap_service,
      mojo::PendingReceiver<brave_wallet::mojom::AssetRatioService>
          asset_ratio_service,
      mojo::PendingReceiver<brave_wallet::mojom::KeyringService>
          keyring_service,
      mojo::PendingReceiver<brave_wallet::mojom::BlockchainRegistry>
          blockchain_registry,
      mojo::PendingReceiver<brave_wallet::mojom::TxService> tx_service,
      mojo::PendingReceiver<brave_wallet::mojom::EthTxManagerProxy>
          eth_tx_manager_proxy,
      mojo::PendingReceiver<brave_wallet::mojom::SolanaTxManagerProxy>
          solana_tx_manager_proxy,
      mojo::PendingReceiver<brave_wallet::mojom::FilTxManagerProxy>
          filecoin_tx_manager_proxy,
      mojo::PendingReceiver<brave_wallet::mojom::BraveWalletService>
          brave_wallet_service,
      mojo::PendingReceiver<brave_wallet::mojom::BraveWalletP3A>
          brave_wallet_p3a,
      mojo::PendingReceiver<brave_wallet::mojom::WalletPinService>
          brave_wallet_pin_service_receiver,
      mojo::PendingReceiver<brave_wallet::mojom::WalletAutoPinService>
          brave_wallet_auto_pin_service_receiver,
      mojo::PendingReceiver<brave_wallet::mojom::IpfsService>
          ipfs_service_receiver) override;

  std::unique_ptr<SwapPageHandler> page_handler_;
  std::unique_ptr<WalletHandler> wallet_handler_;

  mojo::Receiver<brave_wallet::mojom::PageHandlerFactory>
      page_factory_receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_ANDROID_SWAP_PAGE_UI_H_
