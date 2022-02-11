/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_OBSERVER_H_

#include "brave/components/brave_vpn/brave_vpn.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"

class BraveVpnService;

class BraveVPNServiceObserver : public brave_vpn::mojom::ServiceObserver {
 public:
  BraveVPNServiceObserver();
  ~BraveVPNServiceObserver() override;
  BraveVPNServiceObserver(const BraveVPNServiceObserver&) = delete;
  BraveVPNServiceObserver& operator=(const BraveVPNServiceObserver&) = delete;

  void Observe(BraveVpnService* service);

  // brave_vpn::mojom::ServiceObserver overrides:
  void OnPurchasedStateChanged(
      brave_vpn::mojom::PurchasedState state) override {}
#if !defined(OS_ANDROID)
  void OnConnectionStateChanged(
      brave_vpn::mojom::ConnectionState state) override {}
  void OnConnectionCreated() override {}
  void OnConnectionRemoved() override {}
#endif

 private:
  mojo::Receiver<brave_vpn::mojom::ServiceObserver> receiver_{this};
};

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_OBSERVER_H_
