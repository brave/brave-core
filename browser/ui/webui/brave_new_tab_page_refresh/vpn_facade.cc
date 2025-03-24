// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/vpn_facade.h"

#include "brave/components/constants/pref_names.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "components/tab_collections/public/tab_interface.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/ui/brave_vpn/brave_vpn_controller.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#endif

namespace brave_new_tab_page_refresh {

#if BUILDFLAG(ENABLE_BRAVE_VPN)

VPNFacade::VPNFacade(tabs::TabInterface& tab,
                     brave_vpn::BraveVpnService* vpn_service)
    : tab_(tab), vpn_service_(vpn_service) {}

VPNFacade::~VPNFacade() = default;

void VPNFacade::ReloadPurchasedState() {
  if (vpn_service_) {
    vpn_service_->ReloadPurchasedState();
  }
}

void VPNFacade::OpenPanel() {
  tab_->GetBrowserWindowInterface()
      ->GetFeatures()
      .brave_vpn_controller()
      ->ShowBraveVPNBubble(/* show_select */ true);
}

void VPNFacade::OpenAccountPage(brave_vpn::mojom::ManageURLType url_type) {
  tab_->GetBrowserWindowInterface()
      ->GetFeatures()
      .brave_vpn_controller()
      ->OpenVPNAccountPage(url_type);
}

void VPNFacade::RecordWidgetUsage() {
  if (vpn_service_) {
    vpn_service_->brave_vpn_metrics()->RecordWidgetUsage(true);
  }
}

std::optional<std::string> VPNFacade::GetWidgetPrefName() {
  return kNewTabPageShowBraveVPN;
}

#endif

}  // namespace brave_new_tab_page_refresh
