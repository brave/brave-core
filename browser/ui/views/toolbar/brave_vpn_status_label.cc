/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/brave_vpn_status_label.h"

#include <utility>

#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/components/brave_vpn/brave_vpn_service_desktop.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/theme_provider.h"

BraveVPNStatusLabel::BraveVPNStatusLabel(Browser* browser)
    : browser_(browser),
      service_(BraveVpnServiceFactory::GetForProfile(browser_->profile())) {
  mojo::PendingRemote<brave_vpn::mojom::ServiceObserver> listener;
  receiver_.Bind(listener.InitWithNewPipeAndPassReceiver());
  service_->AddObserver(std::move(listener));

  SetAutoColorReadabilityEnabled(false);
  UpdateState();

  if (auto* provider =
          BrowserView::GetBrowserViewForBrowser(browser_)->GetThemeProvider()) {
    SetEnabledColor(provider->GetColor(
        BraveThemeProperties::COLOR_MENU_ITEM_SUB_TEXT_COLOR));
  }
}

BraveVPNStatusLabel::~BraveVPNStatusLabel() = default;

void BraveVPNStatusLabel::OnConnectionStateChanged(ConnectionState state) {
  UpdateState();
}

void BraveVPNStatusLabel::UpdateState() {
  const auto state = service_->connection_state();
  int string_id = IDS_BRAVE_VPN_STATUS_LABEL_DISCONNECTED;
  switch (state) {
    case ConnectionState::CONNECTING:
      string_id = IDS_BRAVE_VPN_STATUS_LABEL_CONNECTING;
      break;
    case ConnectionState::CONNECTED:
      string_id = IDS_BRAVE_VPN_STATUS_LABEL_CONNECTED;
      break;
    case ConnectionState::DISCONNECTING:
      string_id = IDS_BRAVE_VPN_STATUS_LABEL_DISCONNECTING;
      break;
    default:
      break;
  }

  SetText(l10n_util::GetStringUTF16(string_id));
}
