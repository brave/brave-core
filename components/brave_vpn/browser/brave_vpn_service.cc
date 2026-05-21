/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/brave_vpn_service.h"

namespace brave_vpn {

BraveVpnService::BraveVpnService() = default;

BraveVpnService::~BraveVpnService() = default;

void BraveVpnService::AddObserver(
    mojo::PendingRemote<mojom::ServiceObserver> observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.Add(std::move(observer));
}

void BraveVpnService::BindInterface(
    mojo::PendingReceiver<mojom::ServiceHandler> receiver) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  receivers_.Add(this, std::move(receiver));
}

#if BUILDFLAG(IS_ANDROID)
mojo::PendingRemote<brave_vpn::mojom::ServiceHandler>
BraveVpnService::MakeRemote() {
  mojo::PendingRemote<brave_vpn::mojom::ServiceHandler> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}
#endif  // BUILDFLAG(IS_ANDROID)

void BraveVpnService::Shutdown() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.Clear();
#if !BUILDFLAG(IS_ANDROID)
  receivers_.Clear();
#endif  // !BUILDFLAG(IS_ANDROID)
}

void BraveVpnService::NotifyPurchasedStateChanged(
    mojom::PurchasedState state,
    const std::optional<std::string>& description) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  for (const auto& obs : observers_) {
    obs->OnPurchasedStateChanged(state, description);
  }
}

#if !BUILDFLAG(IS_ANDROID)
void BraveVpnService::NotifyConnectionStateChanged(
    mojom::ConnectionState state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  for (const auto& obs : observers_) {
    obs->OnConnectionStateChanged(state);
  }
}

void BraveVpnService::NotifySelectedRegionChanged(mojom::RegionPtr region) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  for (const auto& obs : observers_) {
    obs->OnSelectedRegionChanged(region.Clone());
  }
}

void BraveVpnService::NotifySmartProxyRoutingStateChanged(bool enabled) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  for (const auto& obs : observers_) {
    obs->OnSmartProxyRoutingStateChanged(enabled);
  }
}
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace brave_vpn
