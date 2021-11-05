/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/trezor/trezor_ui.h"

#include <string>

#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/trezor_bridge/resources/grit/trezor_bridge_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"

namespace {
const char kTrezorConnectURL[] = "https://connect.trezor.io/";
}  // namespace

namespace trezor {

UntrustedTrezorUI::UntrustedTrezorUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui) {
  auto* untrusted_source =
      content::WebUIDataSource::Create(kUntrustedTrezorURL);
  untrusted_source->SetDefaultResource(IDR_BRAVE_WALLET_TREZOR_BRIDGE_HTML);
  untrusted_source->AddResourcePaths(
      base::make_span(kTrezorBridgeGenerated, kTrezorBridgeGeneratedSize));
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPageURL));
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPanelURL));
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      std::string("script-src 'self' ") + kTrezorConnectURL + ";");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      std::string("frame-src ") + kTrezorConnectURL + ";");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      std::string("style-src 'unsafe-inline';"));
  auto* browser_context = web_ui->GetWebContents()->GetBrowserContext();
  content::WebUIDataSource::Add(browser_context, untrusted_source);
}

UntrustedTrezorUI::~UntrustedTrezorUI() = default;

std::unique_ptr<content::WebUIController>
UntrustedTrezorUIConfig::CreateWebUIController(content::WebUI* web_ui) {
  return std::make_unique<UntrustedTrezorUI>(web_ui);
}

UntrustedTrezorUIConfig::UntrustedTrezorUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kUntrustedTrezorHost) {}

}  // namespace trezor
