/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_OBSERVER_SERVICE_WIN_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_OBSERVER_SERVICE_WIN_H_

#include <optional>
#include <utility>

#include "brave/components/brave_vpn/browser/brave_vpn_service_observer.h"
#include "components/keyed_service/core/keyed_service.h"

namespace brave_vpn {

class BraveVpnWireguardObserverService
    : public brave_vpn::BraveVPNServiceObserver,
      public KeyedService {
 public:
  BraveVpnWireguardObserverService();
  ~BraveVpnWireguardObserverService() override;
  BraveVpnWireguardObserverService(const BraveVpnWireguardObserverService&) =
      delete;
  BraveVpnWireguardObserverService operator=(
      const BraveVpnWireguardObserverService&) = delete;

  // brave_vpn::BraveVPNServiceObserver
  void OnConnectionStateChanged(
      brave_vpn::mojom::ConnectionState state) override;

 private:
  friend class BraveVpnWireguardObserverServiceUnitTest;

  void SetDialogCallbackForTesting(base::RepeatingClosure callback) {
    dialog_callback_ = std::move(callback);
  }

  void SetFallbackForTesting(bool should_fallback_for_testing) {
    should_fallback_for_testing_ = should_fallback_for_testing;
  }

  void ShowFallbackDialog();
  bool ShouldShowFallbackDialog() const;

  std::optional<bool> should_fallback_for_testing_;
  base::RepeatingClosure dialog_callback_;
};

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_OBSERVER_SERVICE_WIN_H_
