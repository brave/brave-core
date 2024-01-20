/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/brave_vpn_toggle_button.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_id.h"

using ConnectionState = brave_vpn::mojom::ConnectionState;
using PurchasedState = brave_vpn::mojom::PurchasedState;

BraveVPNToggleButton::BraveVPNToggleButton(Browser* browser)
    : browser_(browser),
      service_(brave_vpn::BraveVpnServiceFactory::GetForProfile(
          browser_->profile())) {
  CHECK(service_);

  Observe(service_);

  SetCallback(base::BindRepeating(&BraveVPNToggleButton::OnButtonPressed,
                                  base::Unretained(this)));
  UpdateState();

  if (const ui::ColorProvider* provider =
          BrowserView::GetBrowserViewForBrowser(browser_)->GetColorProvider()) {
    SetThumbOnColor(provider->GetColor(ui::kColorToggleButtonThumbOn));
    SetThumbOffColor(provider->GetColor(ui::kColorToggleButtonThumbOff));
    SetTrackOnColor(provider->GetColor(ui::kColorToggleButtonTrackOn));
    SetTrackOffColor(provider->GetColor(ui::kColorToggleButtonTrackOff));
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
  const auto state = service_->GetConnectionState();
  bool is_on = (state == ConnectionState::CONNECTING ||
                state == ConnectionState::CONNECTED);
  SetIsOn(is_on);
}

BEGIN_METADATA(BraveVPNToggleButton)
END_METADATA
