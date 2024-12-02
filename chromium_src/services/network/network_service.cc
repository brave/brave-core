/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "services/network/network_service.h"

#define UpdateKeyPinsList UpdateKeyPinsList_Unused
#include "src/services/network/network_service.cc"
#undef UpdateKeyPinsList

#include "brave/net/dns/secure_dns_counter.h"
#include "services/network/public/mojom/network_service.mojom.h"

namespace network {

void NetworkService::GetDnsRequestCountsAndReset(
    mojom::NetworkService::GetDnsRequestCountsAndResetCallback callback) {
  net::DnsRequestCounts counts =
      net::SecureDnsCounter::GetInstance()->GetCountsAndReset();
  std::move(callback).Run(
      mojom::DnsRequestCounts::New(counts.total_count, counts.upgraded_count));
}

void NetworkService::UpdateKeyPinsList(mojom::PinListPtr pin_list,
                                       base::Time update_time) {}

}  // namespace network
