/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/ledger/ledger_ui.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_page/wallet_page_ui.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_panel/wallet_panel_ui.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "brave/components/ledger_bridge/resources/grit/ledger_bridge_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_data_source.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/webui/resources/grit/webui_resources.h"

namespace ledger {

UntrustedLedgerUI::UntrustedLedgerUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui),
      ui::EnableMojoWebUI(web_ui, false, false) {
  auto* untrusted_source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kUntrustedLedgerURL);
  untrusted_source->SetDefaultResource(IDR_BRAVE_WALLET_LEDGER_BRIDGE_HTML);
  untrusted_source->AddResourcePaths(kLedgerBridgeGenerated);
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPageURL));
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPanelURL));
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self' chrome-untrusted://resources;");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc, "connect-src 'self';");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::TrustedTypes,
      "trusted-types svelte-trusted-html default;");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      std::string("style-src 'unsafe-inline';"));
  untrusted_source->AddResourcePath("load_time_data_deprecated.js",
                                    IDR_WEBUI_JS_LOAD_TIME_DATA_DEPRECATED_JS);
  untrusted_source->UseStringsJs();
  untrusted_source->AddString("braveWalletLedgerBridgeUrl",
                              kUntrustedLedgerURL);
  untrusted_source->AddBoolean("isLedgerMojoBridgeEnabled",
                               brave_wallet::IsMojoForHardwareWalletEnabled());
}

UntrustedLedgerUI::~UntrustedLedgerUI() = default;

void UntrustedLedgerUI::BindInterface(
    mojo::PendingReceiver<brave_wallet::mojom::LedgerBridgeUIHandler>
        receiver) {
  receiver_.Bind(std::move(receiver));
}

void UntrustedLedgerUI::BindLedgerBridge(
    mojo::PendingRemote<brave_wallet::mojom::LedgerBridge> bridge) {
  content::RenderFrameHost* rfh =
      web_ui()->GetWebContents()->GetPrimaryMainFrame();
  if (!rfh) {
    return;
  }

  // The ledger frame is only ever embedded by the wallet page or panel
  // WebUIs.
  CHECK(rfh->GetWebUI());
  content::WebUIController* controller = rfh->GetWebUI()->GetController();

  if (auto* page = controller->GetAs<brave_wallet::WalletPageUI>()) {
    page->BindLedgerBridge(std::move(bridge));
    return;
  }
  if (auto* panel = controller->GetAs<WalletPanelUI>()) {
    panel->BindLedgerBridge(std::move(bridge));
    return;
  }
}

WEB_UI_CONTROLLER_TYPE_IMPL(UntrustedLedgerUI)

std::unique_ptr<content::WebUIController>
UntrustedLedgerUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                               const GURL& url) {
  return std::make_unique<UntrustedLedgerUI>(web_ui);
}

UntrustedLedgerUIConfig::UntrustedLedgerUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kUntrustedLedgerHost) {}

}  // namespace ledger
