/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/snap_host/snap_host_ui.h"

#include <memory>

#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "brave/components/snap_host/resources/grit/snap_host_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "url/gurl.h"

namespace snap_host {

UntrustedSnapHostUI::UntrustedSnapHostUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui) {
  auto* untrusted_source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kUntrustedSnapURL);

  untrusted_source->SetDefaultResource(IDR_BRAVE_WALLET_SNAP_HOST_HTML);
  untrusted_source->AddResourcePaths(kSnapHostGenerated);

  // v1 is wallet-page-only; the panel's real URL is
  // chrome://wallet-panel.top-chrome/, not chrome://wallet-panel/, and
  // snap_host.ts hardcodes WALLET_PAGE_ORIGIN = 'chrome://wallet' anyway.
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPageURL));

  // 'unsafe-eval' is required for new Function() evaluation of snap bundles.
  // 'unsafe-inline' is unnecessary because snap_host.html loads an external
  // script.
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self' 'unsafe-eval';");
  // new Function(string) throws under require-trusted-types-for regardless of
  // 'unsafe-eval'; clear that directive and the default trusted-types policy.
  untrusted_source->DisableTrustedTypesCSP();

  // connect-src is deliberately not overridden, so it falls back to the
  // chrome-untrusted default-src 'self' and the host cannot reach the
  // network.
  // TODO(https://github.com/brave/brave-browser/issues/58686): allow
  // connect-src once endowment:network-access is enforced per snap.

  // These restate chrome-untrusted defaults: frame-src and worker-src inherit
  // child-src 'none', while form-action explicitly defaults to 'none'. Spelled
  // out so they cannot silently relax if the upstream defaults change.
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc, "frame-src 'none';");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::WorkerSrc, "worker-src 'none';");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FormAction, "form-action 'none';");
}

UntrustedSnapHostUI::~UntrustedSnapHostUI() = default;

bool UntrustedSnapHostUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return brave_wallet::IsSnapFeatureEnabled();
}

std::unique_ptr<content::WebUIController>
UntrustedSnapHostUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                                 const GURL& url) {
  return std::make_unique<UntrustedSnapHostUI>(web_ui);
}

UntrustedSnapHostUIConfig::UntrustedSnapHostUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kUntrustedSnapHost) {}

}  // namespace snap_host
