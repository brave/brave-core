// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BRAVE_DOMAINS_URLS_H_
#define BRAVE_BRAVE_DOMAINS_URLS_H_

#include "url/gurl.h"

namespace brave_domains {

// This file is for URLs shared across multiple components with no clear single
// owner. Feature-specific URLs should be colocated in their feature's target.

// Returns the gate3 URL for the current environment.
// Uses GetServicesDomain("gate3.wallet") with https:// scheme.
//
// Environment is selected via CLI switches:
//   --env-gate3.wallet={dev,staging,prod}  (prefix-specific)
//   --brave-services-env={dev,staging,prod} (global fallback)
//
//   DEV:     https://gate3.wallet.brave.software
//   STAGING: https://gate3.wallet.bravesoftware.com
//   PROD:    https://gate3.wallet.brave.com
GURL GetGate3URL();

}  // namespace brave_domains

#endif  // BRAVE_BRAVE_DOMAINS_URLS_H_
