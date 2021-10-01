/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/wallet_panel_ui.h"

#include <utility>

#include "base/bind.h"
#include "brave/browser/brave_wallet/asset_ratio_controller_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/eth_tx_controller_factory.h"
#include "brave/browser/brave_wallet/keyring_controller_factory.h"
#include "brave/browser/brave_wallet/rpc_controller_factory.h"
#include "brave/browser/brave_wallet/swap_controller_factory.h"
#include "brave/browser/brave_wallet/trezor_bridge_controller_factory.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_common_ui.h"
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
#include "brave/components/brave_wallet_panel/resources/grit/brave_wallet_panel_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/webui/web_ui_util.h"

namespace {

content::WebContents* GetWebContentsFromTabId(int32_t tab_id) {
  for (auto* browser : *BrowserList::GetInstance()) {
    TabStripModel* tab_strip_model = browser->tab_strip_model();
    for (int index = 0; index < tab_strip_model->count(); ++index) {
      content::WebContents* contents = tab_strip_model->GetWebContentsAt(index);
      if (sessions::SessionTabHelper::IdForTab(contents).id() == tab_id) {
        return contents;
      }
    }
  }

  return nullptr;
}

}  // namespace

WalletPanelUI::WalletPanelUI(content::WebUI* web_ui)
    : ui::MojoBubbleWebUIController(web_ui,
                                    true /* Needed for webui browser tests */) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(kWalletPanelHost);

  source->AddLocalizedStrings(brave_wallet::kLocalizedStrings);
  webui::SetupWebUIDataSource(source,
                              base::make_span(kBraveWalletPanelGenerated,
                                              kBraveWalletPanelGeneratedSize),
                              IDR_WALLET_PANEL_HTML);
  auto* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource::Add(profile, source);
  brave_wallet::AddERCTokenImageSource(profile);
}

WalletPanelUI::~WalletPanelUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(WalletPanelUI)

void WalletPanelUI::BindInterface(
    mojo::PendingReceiver<brave_wallet::mojom::PanelHandlerFactory> receiver) {
  panel_factory_receiver_.reset();
  panel_factory_receiver_.Bind(std::move(receiver));
}

void WalletPanelUI::CreatePanelHandler(
    mojo::PendingRemote<brave_wallet::mojom::Page> page,
    mojo::PendingReceiver<brave_wallet::mojom::PanelHandler> panel_receiver,
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

  panel_handler_ = std::make_unique<WalletPanelHandler>(
      std::move(panel_receiver), this,
      base::BindRepeating(&GetWebContentsFromTabId));
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
