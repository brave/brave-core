/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/brave_vpn_service_observer.h"

#include <utility>

#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"

namespace brave_vpn {

BraveVPNServiceObserver::BraveVPNServiceObserver() = default;

BraveVPNServiceObserver::~BraveVPNServiceObserver() = default;

void BraveVPNServiceObserver::Observe(BraveVpnService* service) {
  if (!service)
    return;

  if (service->IsBraveVPNEnabled()) {
    mojo::PendingRemote<mojom::ServiceObserver> listener;
    receiver_.Bind(listener.InitWithNewPipeAndPassReceiver());
    service->AddObserver(std::move(listener));
  }
}

}  // namespace brave_vpn
