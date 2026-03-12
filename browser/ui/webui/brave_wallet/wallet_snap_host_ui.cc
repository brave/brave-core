/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/wallet_snap_host_ui.h"

#include <utility>

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/snaps_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "base/logging.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/snap_host/resources/grit/snap_host_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"

WalletSnapHostUI::WalletSnapHostUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui, /*enable_chrome_send=*/false) {
  LOG(ERROR) << "XXXZZZ WalletSnapHostUI: constructed";
  auto* profile = Profile::FromWebUI(web_ui);
  auto* source = content::WebUIDataSource::CreateAndAdd(
      profile, kWalletSnapHostHost);

  // Allow embedding chrome-untrusted://snap-executor iframes.
  web_ui->AddRequestableScheme(content::kChromeUIUntrustedScheme);
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      std::string("frame-src ") + kUntrustedSnapExecutorURL + ";");
  // The host page itself only loads its own scripts — no inline eval needed.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self';");

  source->UseStringsJs();
  source->SetDefaultResource(IDR_BRAVE_WALLET_SNAP_HOST_HTML);
  source->AddResourcePaths(kSnapHostGenerated);
}

WalletSnapHostUI::~WalletSnapHostUI() = default;
WEB_UI_CONTROLLER_TYPE_IMPL(WalletSnapHostUI)

void WalletSnapHostUI::BindInterface(
    mojo::PendingReceiver<brave_wallet::mojom::WalletSnapHostHandlerFactory>
        receiver) {
  factory_receiver_.reset();
  factory_receiver_.Bind(std::move(receiver));
}

void WalletSnapHostUI::CreateSnapHostHandler(
    mojo::PendingRemote<brave_wallet::mojom::SnapBridge> snap_bridge,
    mojo::PendingReceiver<brave_wallet::mojom::SnapRequestHandler>
        snap_request_handler,
    mojo::PendingReceiver<brave_wallet::mojom::SnapsService> snaps_service) {
  LOG(ERROR) << "XXXZZZ WalletSnapHostUI::CreateSnapHostHandler: called";
  auto* profile = Profile::FromWebUI(web_ui());
  if (!profile) {
    LOG(ERROR) << "XXXZZZ WalletSnapHostUI::CreateSnapHostHandler: no profile";
    return;
  }
  auto* wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForContext(profile);
  if (!wallet_service) {
    LOG(ERROR) << "XXXZZZ WalletSnapHostUI::CreateSnapHostHandler: no wallet_service";
    return;
  }
  if (auto* snaps = wallet_service->snaps_service()) {
    LOG(ERROR) << "XXXZZZ WalletSnapHostUI::CreateSnapHostHandler: wiring bridge";
    snaps->SetSnapBridge(std::move(snap_bridge));
    snaps->BindSnapRequestHandler(std::move(snap_request_handler));
    snaps->Bind(std::move(snaps_service));
  } else {
    LOG(ERROR) << "XXXZZZ WalletSnapHostUI::CreateSnapHostHandler: no snaps_service";
  }
}

WalletSnapHostUIConfig::WalletSnapHostUIConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme, kWalletSnapHostHost) {}

bool WalletSnapHostUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return brave_wallet::IsAllowedForContext(browser_context);
}
