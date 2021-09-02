// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_vpn/vpn_panel_ui.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_vpn/resources/panel/grit/brave_vpn_panel_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/grit/brave_components_resources.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"

VPNPanelUI::VPNPanelUI(content::WebUI* web_ui)
    : ui::MojoBubbleWebUIController(web_ui, true) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(kVPNPanelHost);

  webui::SetupWebUIDataSource(
      source,
      base::make_span(kBraveVpnPanelGenerated, kBraveVpnPanelGeneratedSize),
      IDR_VPN_PANEL_HTML);
  content::WebUIDataSource::Add(web_ui->GetWebContents()->GetBrowserContext(),
                                source);
  Profile* profile = Profile::FromWebUI(web_ui);
  content::URLDataSource::Add(
      profile, std::make_unique<FaviconSource>(
                   profile, chrome::FaviconUrlFormat::kFavicon2));
}

VPNPanelUI::~VPNPanelUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(VPNPanelUI)

void VPNPanelUI::BindInterface(
    mojo::PendingReceiver<brave_vpn::mojom::PanelHandlerFactory> receiver) {
  panel_factory_receiver_.reset();
  panel_factory_receiver_.Bind(std::move(receiver));
}

void VPNPanelUI::CreatePanelHandler(
    mojo::PendingRemote<brave_vpn::mojom::Page> page,
    mojo::PendingReceiver<brave_vpn::mojom::PanelHandler> panel_receiver,
    mojo::PendingReceiver<brave_vpn::mojom::ServiceHandler>
        vpn_service_receiver) {
  DCHECK(page);
  auto* profile = Profile::FromWebUI(web_ui());
  DCHECK(profile);

  panel_handler_ =
      std::make_unique<VPNPanelHandler>(std::move(panel_receiver), this);

  BraveVpnServiceDesktop* vpn_service_desktop =
      BraveVpnServiceFactory::GetForProfile(profile);
  if (vpn_service_desktop) {
    vpn_service_desktop->BindInterface(std::move(vpn_service_receiver));
  }
}
