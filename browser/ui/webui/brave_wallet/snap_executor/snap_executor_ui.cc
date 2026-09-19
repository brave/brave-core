/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/snap_executor/snap_executor_ui.h"

#include <memory>

#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "brave/components/snap_executor/resources/grit/snap_executor_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "url/gurl.h"

namespace snap_executor {

UntrustedSnapExecutorUI::UntrustedSnapExecutorUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui) {
  auto* untrusted_source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kUntrustedSnapExecutorURL);

  untrusted_source->SetDefaultResource(IDR_BRAVE_WALLET_SNAP_EXECUTOR_HTML);
  untrusted_source->AddResourcePaths(kSnapExecutorGenerated);

  // v1 is wallet-page-only; the panel's real URL is
  // chrome://wallet-panel.top-chrome/, not chrome://wallet-panel/, and
  // snap_executor.ts hardcodes PARENT_ORIGIN = 'chrome://wallet' anyway.
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPageURL));

  // 'unsafe-eval' is required for new Function() evaluation of snap bundles.
  // 'unsafe-inline' is intentionally omitted — snap_executor.html loads one
  // external script, and allowing inline scripts would let evaluated snap
  // code inject <script> elements.
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self' 'unsafe-eval';");
  // new Function(string) throws under require-trusted-types-for regardless of
  // 'unsafe-eval'; clear both that directive and the default trusted-types;.
  untrusted_source->DisableTrustedTypesCSP();
  // TODO(snaps): reintroduce connect-src * when endowment:network-access is
  // enforced. Until then, fall back to default-src 'self'.
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc, "frame-src 'none';");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::WorkerSrc, "worker-src 'none';");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FormAction, "form-action 'none';");
}

UntrustedSnapExecutorUI::~UntrustedSnapExecutorUI() = default;

bool UntrustedSnapExecutorUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return brave_wallet::IsSnapsFeatureEnabled();
}

std::unique_ptr<content::WebUIController>
UntrustedSnapExecutorUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                                     const GURL& url) {
  return std::make_unique<UntrustedSnapExecutorUI>(web_ui);
}

UntrustedSnapExecutorUIConfig::UntrustedSnapExecutorUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme,
                  kUntrustedSnapExecutorHost) {}

}  // namespace snap_executor
