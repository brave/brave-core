/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/unstoppable_domains/utils.h"

#include "base/feature_list.h"
#include "base/strings/string_util.h"
#include "brave/components/unstoppable_domains/constants.h"
#include "brave/components/unstoppable_domains/features.h"
#include "brave/components/unstoppable_domains/pref_names.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace unstoppable_domains {

bool IsUnstoppableDomainsTLD(const GURL& url) {
  return base::EndsWith(url.host_piece(), kCryptoDomain);
}

bool IsUnstoppableDomainsEnabled() {
  return base::FeatureList::IsEnabled(features::kUnstoppableDomains);
}

bool IsResolveMethodAsk(PrefService* local_state) {
  if (!local_state || !IsUnstoppableDomainsEnabled()) {
    return false;  // Treat it as disabled.
  }

  return local_state->GetInteger(kResolveMethod) ==
         static_cast<int>(ResolveMethodTypes::ASK);
}

bool IsResolveMethodDoH(PrefService* local_state) {
  if (!local_state || !IsUnstoppableDomainsEnabled()) {
    return false;  // Treat it as disabled.
  }

  return local_state->GetInteger(kResolveMethod) ==
         static_cast<int>(ResolveMethodTypes::DNS_OVER_HTTPS);
}

}  // namespace unstoppable_domains
