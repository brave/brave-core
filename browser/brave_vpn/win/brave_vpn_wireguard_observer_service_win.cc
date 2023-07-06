/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_observer_service_win.h"

#include "brave/browser/ui/views/brave_vpn/brave_vpn_fallback_dialog_view.h"
#include "brave/components/brave_vpn/common/wireguard/win/storage_utils.h"
#include "chrome/browser/ui/browser_finder.h"

namespace brave_vpn {

BraveVpnWireguardObserverService::BraveVpnWireguardObserverService() = default;

BraveVpnWireguardObserverService::~BraveVpnWireguardObserverService() = default;

void BraveVpnWireguardObserverService::ShowFallbackDialog() {
  if (dialog_callback_) {
    dialog_callback_.Run();
    return;
  }
  BraveVpnFallbackDialogView::Show(chrome::FindLastActive());
}

void BraveVpnWireguardObserverService::OnConnectionStateChanged(
    brave_vpn::mojom::ConnectionState state) {
  if (state == brave_vpn::mojom::ConnectionState::DISCONNECTED ||
      state == brave_vpn::mojom::ConnectionState::CONNECT_FAILED) {
    if (ShouldShowFallbackDialog()) {
      ShowFallbackDialog();
    }
  }
}

bool BraveVpnWireguardObserverService::ShouldShowFallbackDialog() const {
  if (should_fallback_for_testing_.has_value()) {
    return should_fallback_for_testing_.value();
  }

  return wireguard::ShouldFallbackToIKEv2();
}

}  // namespace brave_vpn
