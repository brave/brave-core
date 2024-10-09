/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_PANEL_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_PANEL_UI_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/webui/brave_wallet/common_handler/wallet_handler.h"
#include "brave/browser/ui/webui/brave_wallet/panel_handler/wallet_panel_handler.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_webui_config.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace content {
class BrowserContext;
}

class WalletPanelUI : public TopChromeWebUIController,
                      public brave_wallet::mojom::PanelHandlerFactory {
 public:
  explicit WalletPanelUI(content::WebUI* web_ui);
  WalletPanelUI(const WalletPanelUI&) = delete;
  WalletPanelUI& operator=(const WalletPanelUI&) = delete;
  ~WalletPanelUI() override;

  // Instantiates the implementor of the mojom::PanelHandlerFactory mojo
  // interface passing the pending receiver that will be internally bound.
  void BindInterface(
      mojo::PendingReceiver<brave_wallet::mojom::PanelHandlerFactory> receiver);
  // The bubble disappears by default when Trezor opens a popup window
  // from the wallet panel bubble. In order to prevent it we set a callback
  // to modify panel deactivation flag when necessary.
  void SetDeactivationCallback(
      base::RepeatingCallback<void(bool)> deactivation_callback);

  static constexpr std::string GetWebUIName() { return "WalletPanel"; }

 private:
  // brave_wallet::mojom::PanelHandlerFactory:
  void CreatePanelHandler(
      mojo::PendingReceiver<brave_wallet::mojom::PanelHandler> panel_receiver,
      mojo::PendingReceiver<brave_wallet::mojom::WalletHandler> wallet_receiver,
      mojo::PendingReceiver<brave_wallet::mojom::JsonRpcService>
          json_rpc_service,
      mojo::PendingReceiver<brave_wallet::mojom::BitcoinWalletService>
          bitcoin_rpc_service,
      mojo::PendingReceiver<brave_wallet::mojom::ZCashWalletService>
          zcash_service,
      mojo::PendingReceiver<brave_wallet::mojom::SwapService> swap_service,
      mojo::PendingReceiver<brave_wallet::mojom::SimulationService>
          simulation_service,
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
          fil_tx_manager_proxy,
      mojo::PendingReceiver<brave_wallet::mojom::BtcTxManagerProxy>
          bitcoin_tx_manager_proxy_receiver,
      mojo::PendingReceiver<brave_wallet::mojom::BraveWalletService>
          brave_wallet_service,
      mojo::PendingReceiver<brave_wallet::mojom::BraveWalletP3A>
          brave_wallet_p3a,
      mojo::PendingReceiver<brave_wallet::mojom::IpfsService>
          brave_wallet_ipfs_service_receiver,
      mojo::PendingReceiver<brave_wallet::mojom::MeldIntegrationService>
          meld_integration_service) override;

  std::unique_ptr<WalletPanelHandler> panel_handler_;
  std::unique_ptr<brave_wallet::WalletHandler> wallet_handler_;
  raw_ptr<content::WebContents> active_web_contents_ = nullptr;

  base::RepeatingCallback<void(bool)> deactivation_callback_;
  mojo::Receiver<brave_wallet::mojom::PanelHandlerFactory>
      panel_factory_receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class WalletPanelUIConfig : public DefaultTopChromeWebUIConfig<WalletPanelUI> {
 public:
  WalletPanelUIConfig();

  // WebUIConfig::
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;

  // TopChromeWebUIConfig::
  bool ShouldAutoResizeHost() override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_PANEL_UI_H_
