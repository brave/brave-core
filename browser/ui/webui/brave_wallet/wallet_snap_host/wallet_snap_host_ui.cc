/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/wallet_snap_host/wallet_snap_host_ui.h"

#include <memory>
#include <utility>

#include "base/strings/strcat.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/snap_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "brave/components/wallet_snap_host/resources/grit/wallet_snap_host_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/webui/webui_util.h"
#include "url/gurl.h"

namespace brave_wallet {

WalletSnapHostUI::WalletSnapHostUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui, /*enable_chrome_send=*/false) {
  auto* profile = Profile::FromWebUI(web_ui);
  auto* source =
      content::WebUIDataSource::CreateAndAdd(profile, kWalletSnapHostHost);
  webui::SetupWebUIDataSource(source, base::span(kWalletSnapHostGenerated),
                              IDR_BRAVE_WALLET_WALLET_SNAP_HOST_HTML);
  // Required for the chrome-untrusted://snap-host child frame.
  web_ui->AddRequestableScheme(content::kChromeUIUntrustedScheme);
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      base::StrCat({"frame-src ", kUntrustedSnapURL, ";"}));
  // Without a frame ancestor the data source serves `frame-ancestors 'none'`
  // and X-Frame-Options: DENY, blocking the wallet page's hidden host iframe.
  source->AddFrameAncestor(GURL(kBraveUIWalletPageURL));
}

WalletSnapHostUI::~WalletSnapHostUI() = default;
WEB_UI_CONTROLLER_TYPE_IMPL(WalletSnapHostUI)

void WalletSnapHostUI::BindInterface(
    mojo::PendingReceiver<mojom::SnapService> receiver) {
  if (!IsSnapFeatureEnabled()) {
    return;
  }
  auto* profile = Profile::FromWebUI(web_ui());
  if (auto* wallet_service =
          BraveWalletServiceFactory::GetServiceForContext(profile)) {
    if (auto* snap_service = wallet_service->snap_service()) {
      snap_service->Bind(std::move(receiver));
    }
  }
}

WalletSnapHostUIConfig::WalletSnapHostUIConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme, kWalletSnapHostHost) {}

bool WalletSnapHostUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  // Reachable both as the wallet page's hidden iframe and as its own tab.
  return IsSnapFeatureEnabled();
}

}  // namespace brave_wallet
