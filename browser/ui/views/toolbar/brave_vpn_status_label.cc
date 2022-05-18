/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/brave_vpn_status_label.h"

#include <utility>

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

namespace {

int GetStringIdForConnectionState(ConnectionState state) {
  switch (state) {
    case ConnectionState::CONNECTING:
      return IDS_BRAVE_VPN_STATUS_LABEL_CONNECTING;
      break;
    case ConnectionState::CONNECTED:
      return IDS_BRAVE_VPN_STATUS_LABEL_CONNECTED;
      break;
    case ConnectionState::DISCONNECTING:
      return IDS_BRAVE_VPN_STATUS_LABEL_DISCONNECTING;
      break;
    default:
      break;
  }
  return IDS_BRAVE_VPN_STATUS_LABEL_DISCONNECTED;
}

int GetLongestStringIdForConnectionState() {
  size_t max = 0;
  int longest_string_id =
      GetStringIdForConnectionState(ConnectionState::DISCONNECTED);
  for (auto state :
       {ConnectionState::CONNECTING, ConnectionState::CONNECTED,
        ConnectionState::DISCONNECTING, ConnectionState::DISCONNECTED}) {
    auto id = GetStringIdForConnectionState(state);
    auto text = brave_l10n::GetLocalizedResourceUTF16String(id);
    if (text.length() > max) {
      max = text.length();
      longest_string_id = id;
    }
  }
  return longest_string_id;
}
}  // namespace

BraveVPNStatusLabel::BraveVPNStatusLabel(Browser* browser)
    : browser_(browser),
      service_(brave_vpn::BraveVpnServiceFactory::GetForProfile(
          browser_->profile())) {
  DCHECK(service_);

  Observe(service_);
  SetAutoColorReadabilityEnabled(false);
  UpdateState();

  if (auto* provider =
          BrowserView::GetBrowserViewForBrowser(browser_)->GetThemeProvider()) {
    SetEnabledColor(provider->GetColor(
        BraveThemeProperties::COLOR_MENU_ITEM_SUB_TEXT_COLOR));
  }
  longest_state_string_id_ = GetLongestStringIdForConnectionState();
}

BraveVPNStatusLabel::~BraveVPNStatusLabel() = default;

void BraveVPNStatusLabel::OnConnectionStateChanged(ConnectionState state) {
  UpdateState();
}

gfx::Size BraveVPNStatusLabel::CalculatePreferredSize() const {
  auto size = views::Label::CalculatePreferredSize();
  if (longest_state_string_id_ == -1)
    return size;
  auto text =
      brave_l10n::GetLocalizedResourceUTF16String(longest_state_string_id_);
  if (text == GetText())
    return size;
  size.set_width(font_list().GetExpectedTextWidth(text.length()) +
                 GetInsets().width());
  size.set_height(GetHeightForWidth(size.width()));
  return size;
}

void BraveVPNStatusLabel::UpdateState() {
  const auto state = service_->connection_state();

  SetText(brave_l10n::GetLocalizedResourceUTF16String(
      GetStringIdForConnectionState(state)));
}
