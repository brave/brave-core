/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/trezor/trezor_ui.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_page/wallet_page_ui.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_panel/wallet_panel_ui.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "brave/components/trezor_bridge/resources/grit/trezor_bridge_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/resources/grit/webui_resources.h"

namespace {
constexpr char kTrezorConnectURL[] = "https://connect.trezor.io/";
}  // namespace

namespace trezor {

UntrustedTrezorUI::UntrustedTrezorUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui),
      ui::EnableMojoWebUI(web_ui, false, false) {
  auto* untrusted_source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kUntrustedTrezorURL);
  untrusted_source->SetDefaultResource(IDR_BRAVE_WALLET_TREZOR_BRIDGE_HTML);
  untrusted_source->AddResourcePaths(kTrezorBridgeGenerated);
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPageURL));
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPanelURL));
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc,
      "connect-src 'self' https://connect.trezor.io;");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      std::string("script-src chrome://resources/js/ "
                  "chrome-untrusted://resources 'self' ") +
          kTrezorConnectURL + ";");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      std::string("frame-src ") + kTrezorConnectURL + ";");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      std::string("style-src 'unsafe-inline';"));
  untrusted_source->AddResourcePath("load_time_data_deprecated.js",
                                    IDR_WEBUI_JS_LOAD_TIME_DATA_DEPRECATED_JS);
  untrusted_source->UseStringsJs();
  untrusted_source->AddString("braveWalletTrezorBridgeUrl",
                              kUntrustedTrezorURL);
  untrusted_source->AddBoolean("isTrezorMojoBridgeEnabled",
                               brave_wallet::IsMojoForHardwareWalletEnabled());
}

UntrustedTrezorUI::~UntrustedTrezorUI() = default;

void UntrustedTrezorUI::BindInterface(
    mojo::PendingReceiver<brave_wallet::mojom::TrezorBridgeUIHandler>
        receiver) {
  receiver_.Bind(std::move(receiver));
}

void UntrustedTrezorUI::BindTrezorBridge(
    mojo::PendingRemote<brave_wallet::mojom::TrezorBridge> bridge) {
  content::RenderFrameHost* rfh =
      web_ui()->GetWebContents()->GetPrimaryMainFrame();
  if (!rfh) {
    return;
  }

  // The trezor frame is only ever embedded by the wallet page or panel
  // WebUIs.
  CHECK(rfh->GetWebUI());
  content::WebUIController* controller = rfh->GetWebUI()->GetController();

  if (auto* page = controller->GetAs<brave_wallet::WalletPageUI>()) {
    page->BindTrezorBridge(std::move(bridge));
    return;
  }
  if (auto* panel = controller->GetAs<WalletPanelUI>()) {
    panel->BindTrezorBridge(std::move(bridge));
    return;
  }
}

WEB_UI_CONTROLLER_TYPE_IMPL(UntrustedTrezorUI)

std::unique_ptr<content::WebUIController>
UntrustedTrezorUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                               const GURL& url) {
  return std::make_unique<UntrustedTrezorUI>(web_ui);
}

UntrustedTrezorUIConfig::UntrustedTrezorUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kUntrustedTrezorHost) {}

}  // namespace trezor
