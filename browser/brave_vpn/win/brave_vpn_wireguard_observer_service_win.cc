/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_observer_service_win.h"

#include "brave/browser/brave_vpn/win/storage_utils.h"
#include "brave/browser/ui/browser_dialogs.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "chrome/browser/browser_process.h"

namespace brave_vpn {

BraveVpnWireguardObserverService::BraveVpnWireguardObserverService() = default;

BraveVpnWireguardObserverService::~BraveVpnWireguardObserverService() = default;

void BraveVpnWireguardObserverService::ShowFallbackDialog() {
  if (dialog_callback_) {
    dialog_callback_.Run();
    return;
  }
  brave::ShowBraveVpnIKEv2FallbackDialog();
}

void BraveVpnWireguardObserverService::OnConnectionStateChanged(
    brave_vpn::mojom::ConnectionState state) {
  // Check because WG settings could be changed in runtime.
  if (!brave_vpn::IsBraveVPNWireguardEnabled(
          g_browser_process->local_state())) {
    return;
  }

  VLOG(2) << __func__ << state;
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

  return ShouldFallbackToIKEv2();
}

}  // namespace brave_vpn
