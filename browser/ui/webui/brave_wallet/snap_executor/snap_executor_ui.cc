/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/snap_executor/snap_executor_ui.h"

#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/snap_executor/resources/grit/snap_executor_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"

namespace snap_executor {

UntrustedSnapExecutorUI::UntrustedSnapExecutorUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui) {
  auto* untrusted_source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kUntrustedSnapExecutorURL);

  untrusted_source->SetDefaultResource(IDR_BRAVE_WALLET_SNAP_EXECUTOR_HTML);
  untrusted_source->AddResourcePaths(kSnapExecutorGenerated);

  // Only allow embedding from wallet page and wallet panel.
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPageURL));
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPanelURL));

  // Strict CSP: snaps must not make network requests or load sub-frames.
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self';");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc,
      "connect-src 'none';");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      "frame-src 'none';");
}

UntrustedSnapExecutorUI::~UntrustedSnapExecutorUI() = default;

std::unique_ptr<content::WebUIController>
UntrustedSnapExecutorUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                                     const GURL& url) {
  return std::make_unique<UntrustedSnapExecutorUI>(web_ui);
}

UntrustedSnapExecutorUIConfig::UntrustedSnapExecutorUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme,
                  kUntrustedSnapExecutorHost) {}

}  // namespace snap_executor
