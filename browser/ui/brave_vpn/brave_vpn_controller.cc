/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_vpn/brave_vpn_controller.h"

#include <string>

#include "base/functional/bind.h"
#include "base/types/to_address.h"
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/browser/ui/views/toolbar/brave_vpn_button.h"
#include "brave/browser/ui/webui/brave_vpn/vpn_panel_ui.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_element_identifiers.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "chrome/browser/ui/views/interaction/browser_elements_views.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "ui/views/view.h"
#include "url/gurl.h"

DEFINE_USER_DATA(BraveVPNController);

BraveVPNController::BraveVPNController(BrowserWindowInterface& browser)
    : browser_(browser),
      scoped_unowned_user_data_(browser.GetUnownedUserDataHost(), *this) {
  pref_change_registrar_.Init(browser.GetProfile()->GetPrefs());
  pref_change_registrar_.Add(
      brave_vpn::prefs::kBraveVPNShowButton,
      base::BindRepeating(&BraveVPNController::ResetBubbleManager,
                          base::Unretained(this)));
}

BraveVPNController::~BraveVPNController() = default;

// static
BraveVPNController* BraveVPNController::From(BrowserWindowInterface* browser) {
  return Get(browser->GetUnownedUserDataHost());
}

void BraveVPNController::ShowBraveVPNBubble(bool show_select) {
  // Windows without a toolbar, such as popups, have nothing to anchor to.
  views::View* anchor_view = GetAnchorView();
  if (!anchor_view) {
    return;
  }

  if (show_select) {
    // Otherwise a bubble that is still alive reopens where it was left.
    ResetBubbleManager();
  }

  if (!webui_bubble_manager_) {
    std::string url = kVPNPanelURL;
    if (show_select) {
      url += "select";
    }
    webui_bubble_manager_ = WebUIBubbleManager::Create<VPNPanelUI>(
        base::to_address(browser_), GURL(url), IDS_BRAVE_VPN_PANEL_NAME);
  }

  if (webui_bubble_manager_->GetBubbleWidget()) {
    webui_bubble_manager_->CloseBubble();
    return;
  }

  webui_bubble_manager_->ShowBubble(anchor_view);
}

void BraveVPNController::OpenVPNAccountPage(
    brave_vpn::mojom::ManageURLType type) {
  auto* vpn_service =
      brave_vpn::BraveVpnServiceFactory::GetForProfile(browser_->GetProfile());
  const auto url =
      GURL(brave_vpn::GetManageUrl(vpn_service->GetCurrentEnvironment()));
  ShowSingletonTab(base::to_address(browser_),
                   brave_vpn::GetManageURLForUIType(type, url));
}

views::View* BraveVPNController::GetAnchorView() {
  auto& elements = *BrowserElementsViews::From(base::to_address(browser_));
  auto* vpn_button = elements.GetViewAs<BraveVPNButton>(
      BraveVPNButton::kBraveVPNButtonElementId);
  if (vpn_button && vpn_button->GetVisible()) {
    return vpn_button;
  }
  return elements.GetView(kToolbarAppMenuButtonElementId);
}

void BraveVPNController::ResetBubbleManager() {
  webui_bubble_manager_.reset();
}
