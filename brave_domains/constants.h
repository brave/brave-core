// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BRAVE_DOMAINS_CONSTANTS_H_
#define BRAVE_BRAVE_DOMAINS_CONSTANTS_H_

namespace brave_domains {

// Gate3 is a Brave backend service used by both rewards (OAuth authentication)
// and wallet (pricing, swap, token management, etc.).
inline constexpr char kGate3URL[] = "https://gate3.wallet.brave.com";

}  // namespace brave_domains

#endif  // BRAVE_BRAVE_DOMAINS_CONSTANTS_H_
