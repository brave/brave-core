// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_vpn/vpn_panel_handler.h"

#include <utility>

#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/components/brave_vpn/brave_vpn_service.h"

VPNPanelHandler::VPNPanelHandler(
    mojo::PendingReceiver<brave_vpn::mojom::PanelHandler> receiver,
    ui::MojoBubbleWebUIController* webui_controller,
    Profile* profile)
    : receiver_(this, std::move(receiver)),
      webui_controller_(webui_controller),
      profile_(profile) {}

VPNPanelHandler::~VPNPanelHandler() = default;

void VPNPanelHandler::ShowUI() {
  auto embedder = webui_controller_->embedder();
  BraveVpnService* vpn_service_desktop =
      BraveVpnServiceFactory::GetForProfile(profile_);
  if (embedder) {
    embedder->ShowUI();
    vpn_service_desktop->OnPanelVisible();
  }
}

void VPNPanelHandler::CloseUI() {
  auto embedder = webui_controller_->embedder();
  if (embedder) {
    embedder->CloseUI();
  }
}
