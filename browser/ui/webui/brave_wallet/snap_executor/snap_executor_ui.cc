/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/snap_executor/snap_executor_ui.h"

#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "url/gurl.h"

namespace snap_executor {

namespace {

inline constexpr char kUntrustedSnapExecutorURL[] =
    "chrome-untrusted://snap-executor/";
inline constexpr char kBraveUIWalletPageURL[] = "chrome://wallet/";
inline constexpr char kBraveUIWalletPanelURL[] = "chrome://wallet-panel/";

}  // namespace

UntrustedSnapExecutorUI::UntrustedSnapExecutorUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui) {
  auto* untrusted_source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kUntrustedSnapExecutorURL);

  // Resource IDs and generated resource maps are provided by the build.
  // Minimal extraction omits the generated resource integration; add
  // SetDefaultResource(IDR_BRAVE_WALLET_SNAP_EXECUTOR_HTML) and the generated
  // JS map here once resources are wired up.

  // Only allow embedding from wallet page and wallet panel.
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPageURL));
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPanelURL));

  // 'unsafe-eval' is required because snap code may be evaluated in an isolated
  // scope inside the untrusted frame.
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self' 'unsafe-eval' 'unsafe-inline';");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::RequireTrustedTypesFor, "");
  // 'self' for snap bundle fetch(); '*' for snap endowment:network-access.
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc, "connect-src 'self' *;");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc, "frame-src 'none';");
}

UntrustedSnapExecutorUI::~UntrustedSnapExecutorUI() = default;

std::unique_ptr<content::WebUIController>
UntrustedSnapExecutorUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                                     const GURL& url) {
  return std::make_unique<UntrustedSnapExecutorUI>(web_ui);
}

UntrustedSnapExecutorUIConfig::UntrustedSnapExecutorUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, "snap-executor") {}

}  // namespace snap_executor
