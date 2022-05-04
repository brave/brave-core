/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/brave_vpn_toggle_button.h"

#include <utility>

#include "base/bind.h"
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/components/brave_vpn/brave_vpn_service.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/base/theme_provider.h"

using ConnectionState = brave_vpn::mojom::ConnectionState;
using PurchasedState = brave_vpn::mojom::PurchasedState;

BraveVPNToggleButton::BraveVPNToggleButton(Browser* browser)
    : browser_(browser),
      service_(brave_vpn::BraveVpnServiceFactory::GetForProfile(
          browser_->profile())) {
  DCHECK(service_);

  Observe(service_);

  SetCallback(base::BindRepeating(&BraveVPNToggleButton::OnButtonPressed,
                                  base::Unretained(this)));
  UpdateState();

  if (auto* provider =
          BrowserView::GetBrowserViewForBrowser(browser_)->GetThemeProvider()) {
    SetThumbOnColor(provider->GetColor(
        BraveThemeProperties::COLOR_TOGGLE_BUTTON_THUMB_ON_COLOR));
    SetThumbOffColor(provider->GetColor(
        BraveThemeProperties::COLOR_TOGGLE_BUTTON_THUMB_OFF_COLOR));
    SetTrackOnColor(provider->GetColor(
        BraveThemeProperties::COLOR_TOGGLE_BUTTON_TRACK_ON_COLOR));
    SetTrackOffColor(provider->GetColor(
        BraveThemeProperties::COLOR_TOGGLE_BUTTON_TRACK_OFF_COLOR));
  }

  // TODO(simonhong): Re-visit this name.
  SetAccessibleName(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_VPN_TOGGLE_MENU_ITEM_TEXT));
}

BraveVPNToggleButton::~BraveVPNToggleButton() = default;

void BraveVPNToggleButton::OnConnectionStateChanged(ConnectionState state) {
  UpdateState();
}

void BraveVPNToggleButton::OnButtonPressed(const ui::Event& event) {
  service_->ToggleConnection();
}

void BraveVPNToggleButton::UpdateState() {
  const auto state = service_->connection_state();
  bool is_on = (state == ConnectionState::CONNECTING ||
                state == ConnectionState::CONNECTED);
  SetIsOn(is_on);
}
