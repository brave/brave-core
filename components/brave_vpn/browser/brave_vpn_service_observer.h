/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_SERVICE_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_SERVICE_OBSERVER_H_

#include <optional>
#include <string>

#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "build/build_config.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace brave_vpn {

class BraveVpnService;

class BraveVPNServiceObserver : public mojom::ServiceObserver {
 public:
  BraveVPNServiceObserver();
  ~BraveVPNServiceObserver() override;
  BraveVPNServiceObserver(const BraveVPNServiceObserver&) = delete;
  BraveVPNServiceObserver& operator=(const BraveVPNServiceObserver&) = delete;

  void Observe(BraveVpnService* service);

  // mojom::ServiceObserver overrides:
  void OnPurchasedStateChanged(
      mojom::PurchasedState state,
      const std::optional<std::string>& description) override {}
#if !BUILDFLAG(IS_ANDROID)
  void OnConnectionStateChanged(mojom::ConnectionState state) override {}
  void OnSelectedRegionChanged(mojom::RegionPtr region) override {}
#endif  // !BUILDFLAG(IS_ANDROID)

 private:
  mojo::Receiver<mojom::ServiceObserver> receiver_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_SERVICE_OBSERVER_H_
