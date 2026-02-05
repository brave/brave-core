/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_CONSTANTS_H_

namespace brave_rewards {

// Gate3 is a Brave backend service used by both rewards (OAuth authentication)
// and wallet (pricing, swap, token management, etc.).
//
// This constant is defined here in rewards rather than wallet because
// browser/net guards its usage with ENABLE_BRAVE_REWARDS. Defining it in wallet
// would require including a wallet header inside a rewards buildflag guard,
// which misrepresents the dependency.
inline constexpr char kGate3URL[] = "https://gate3.wallet.brave.com";

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_CONSTANTS_H_
