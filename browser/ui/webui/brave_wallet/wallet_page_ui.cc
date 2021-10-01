/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/wallet_page_ui.h"

#include <utility>

#include "base/files/file_path.h"
#include "brave/browser/brave_wallet/asset_ratio_controller_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/eth_tx_controller_factory.h"
#include "brave/browser/brave_wallet/keyring_controller_factory.h"
#include "brave/browser/brave_wallet/rpc_controller_factory.h"
#include "brave/browser/brave_wallet/swap_controller_factory.h"
#include "brave/browser/brave_wallet/trezor_bridge_controller_factory.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_common_ui.h"
#include "brave/browser/ui/webui/navigation_bar_data_provider.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_wallet/browser/asset_ratio_controller.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/erc_token_registry.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/eth_tx_controller.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "brave/components/brave_wallet/browser/swap_controller.h"
#include "brave/components/brave_wallet/browser/trezor_bridge_controller.h"
#include "brave/components/brave_wallet_page/resources/grit/brave_wallet_page_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/webui/web_ui_util.h"

WalletPageUI::WalletPageUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui,
                              true /* Needed for webui browser tests */) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(kWalletPageHost);

  source->AddLocalizedStrings(brave_wallet::kLocalizedStrings);
  NavigationBarDataProvider::Initialize(source);
  webui::SetupWebUIDataSource(
      source,
      base::make_span(kBraveWalletPageGenerated, kBraveWalletPageGeneratedSize),
      IDR_WALLET_PAGE_HTML);
  auto* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource::Add(profile, source);
  brave_wallet::AddERCTokenImageSource(profile);
}

WalletPageUI::~WalletPageUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(WalletPageUI)

void WalletPageUI::BindInterface(
    mojo::PendingReceiver<brave_wallet::mojom::PageHandlerFactory> receiver) {
  page_factory_receiver_.reset();
  page_factory_receiver_.Bind(std::move(receiver));
}

void WalletPageUI::CreatePageHandler(
    mojo::PendingRemote<brave_wallet::mojom::Page> page,
    mojo::PendingReceiver<brave_wallet::mojom::PageHandler> page_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::WalletHandler> wallet_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::EthJsonRpcController>
        eth_json_rpc_controller_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::SwapController>
        swap_controller_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::AssetRatioController>
        asset_ratio_controller_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::KeyringController>
        keyring_controller_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::ERCTokenRegistry>
        erc_token_registry_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::EthTxController>
        eth_tx_controller_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::BraveWalletService>
        brave_wallet_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::TrezorBridgeController>
        trezor_controller_receiver) {
  DCHECK(page);
  auto* profile = Profile::FromWebUI(web_ui());
  DCHECK(profile);

  page_handler_ =
      std::make_unique<WalletPageHandler>(std::move(page_receiver), profile);
  wallet_handler_ =
      std::make_unique<WalletHandler>(std::move(wallet_receiver), profile);

  auto* eth_json_rpc_controller =
      brave_wallet::RpcControllerFactory::GetControllerForContext(profile);
  if (eth_json_rpc_controller) {
    eth_json_rpc_controller->Bind(std::move(eth_json_rpc_controller_receiver));
  }

  auto* swap_controller =
      brave_wallet::SwapControllerFactory::GetControllerForContext(profile);
  if (swap_controller) {
    swap_controller->Bind(std::move(swap_controller_receiver));
  }

  auto* asset_ratio_controller =
      brave_wallet::AssetRatioControllerFactory::GetControllerForContext(
          profile);
  if (asset_ratio_controller) {
    asset_ratio_controller->Bind(std::move(asset_ratio_controller_receiver));
  }

  auto* keyring_controller =
      brave_wallet::KeyringControllerFactory::GetControllerForContext(profile);
  if (keyring_controller) {
    keyring_controller->Bind(std::move(keyring_controller_receiver));
  }

  auto* erc_token_registry = brave_wallet::ERCTokenRegistry::GetInstance();
  if (erc_token_registry) {
    erc_token_registry->Bind(std::move(erc_token_registry_receiver));
  }

  auto* eth_tx_controller =
      brave_wallet::EthTxControllerFactory::GetControllerForContext(profile);
  if (eth_tx_controller) {
    eth_tx_controller->Bind(std::move(eth_tx_controller_receiver));
  }

  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForContext(profile);
  if (brave_wallet_service) {
    brave_wallet_service->Bind(std::move(brave_wallet_service_receiver));
  }
  auto* trezor_controller =
      brave_wallet::TrezorBridgeControllerFactory::GetControllerForContext(
          profile);
  if (trezor_controller) {
    trezor_controller->Bind(std::move(trezor_controller_receiver));
  }
}
