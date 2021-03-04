/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/decentralized_dns/utils.h"

#include "base/feature_list.h"
#include "base/strings/string_util.h"
#include "brave/components/decentralized_dns/constants.h"
#include "brave/components/decentralized_dns/features.h"
#include "brave/components/decentralized_dns/pref_names.h"
#include "brave/net/decentralized_dns/constants.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace decentralized_dns {

bool IsDecentralizedDnsEnabled() {
  return base::FeatureList::IsEnabled(features::kDecentralizedDns);
}

bool IsUnstoppableDomainsTLD(const GURL& url) {
  return base::EndsWith(url.host_piece(), kCryptoDomain);
}

bool IsUnstoppableDomainsResolveMethodAsk(PrefService* local_state) {
  if (!local_state || !IsDecentralizedDnsEnabled()) {
    return false;  // Treat it as disabled.
  }

  return local_state->GetInteger(kUnstoppableDomainsResolveMethod) ==
         static_cast<int>(ResolveMethodTypes::ASK);
}

bool IsUnstoppableDomainsResolveMethodDoH(PrefService* local_state) {
  if (!local_state || !IsDecentralizedDnsEnabled()) {
    return false;  // Treat it as disabled.
  }

  return local_state->GetInteger(kUnstoppableDomainsResolveMethod) ==
         static_cast<int>(ResolveMethodTypes::DNS_OVER_HTTPS);
}

bool IsENSTLD(const GURL& url) {
  return base::EndsWith(url.host_piece(), kEthDomain);
}

bool IsENSResolveMethodAsk(PrefService* local_state) {
  if (!local_state || !IsDecentralizedDnsEnabled()) {
    return false;  // Treat it as disabled.
  }

  return local_state->GetInteger(kENSResolveMethod) ==
         static_cast<int>(ResolveMethodTypes::ASK);
}

bool IsENSResolveMethodDoH(PrefService* local_state) {
  if (!local_state || !IsDecentralizedDnsEnabled()) {
    return false;  // Treat it as disabled.
  }

  return local_state->GetInteger(kENSResolveMethod) ==
         static_cast<int>(ResolveMethodTypes::DNS_OVER_HTTPS);
}

}  // namespace decentralized_dns
