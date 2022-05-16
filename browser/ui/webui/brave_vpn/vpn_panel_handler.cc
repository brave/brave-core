// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_vpn/vpn_panel_handler.h"
#include "brave/browser/ui/webui/brave_vpn/vpn_panel_ui.h"

#include <utility>

#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/components/brave_vpn/brave_vpn_constants.h"
#include "brave/components/brave_vpn/brave_vpn_service.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_tabstrip.h"

namespace {

GURL GetURLForUIType(const std::string& type) {
  if (type == "recover" || type == "checkout") {
    GURL manage_url = GURL(brave_vpn::GetManageUrl());
    DCHECK(manage_url.is_valid());
    std::string query = "intent=" + type + "&product=vpn";
    GURL::Replacements replacements;
    replacements.SetQueryStr(query);
    return manage_url.ReplaceComponents(replacements);
  } else if (type == "privacy") {
    return GURL("https://brave.com/privacy/browser/#vpn");
  } else if (type == "about") {
    return GURL(brave_vpn::kAboutUrl);
  }
  DCHECK_EQ(type, "manage");
  return GURL(brave_vpn::GetManageUrl());
}

}  // namespace

VPNPanelHandler::VPNPanelHandler(
    mojo::PendingReceiver<brave_vpn::mojom::PanelHandler> receiver,
    VPNPanelUI* panel_controller,
    Profile* profile)
    : receiver_(this, std::move(receiver)),
      panel_controller_(panel_controller),
      profile_(profile) {}

VPNPanelHandler::~VPNPanelHandler() = default;

void VPNPanelHandler::ShowUI() {
  auto embedder = panel_controller_->embedder();
  brave_vpn::BraveVpnService* vpn_service =
      brave_vpn::BraveVpnServiceFactory::GetForProfile(profile_);
  DCHECK(vpn_service);
  if (embedder) {
    embedder->ShowUI();
    vpn_service->LoadPurchasedState();
  }
}

void VPNPanelHandler::CloseUI() {
  auto embedder = panel_controller_->embedder();
  if (embedder) {
    embedder->CloseUI();
  }
}

void VPNPanelHandler::OpenVpnUI(const std::string& type) {
  auto* browser = chrome::FindLastActiveWithProfile(profile_);
  chrome::AddTabAt(browser, GetURLForUIType(type), -1, true);
}
