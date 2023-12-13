/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DECENTRALIZED_DNS_CORE_PREF_NAMES_H_
#define BRAVE_COMPONENTS_DECENTRALIZED_DNS_CORE_PREF_NAMES_H_

namespace decentralized_dns {

// Used to determine which method should be used to resolve unstoppable
// domains, between:
// Disabled: Disable all unstoppable domains resolution.
// Ask: Ask users if they want to enable support of unstoppable domains.
// Enabled: Resolve domain name using Ethereum JSON-RPC server.
inline constexpr char kUnstoppableDomainsResolveMethod[] =
    "brave.unstoppable_domains.resolve_method";

// Used to determine which method should be used to resolve ENS domains,
// between:
// Disabled: Disable all ENS domains resolution.
// Ask: Ask users if they want to enable support of ENS.
// Enabled: Resolve domain name using Ethereum JSON-RPC server.
inline constexpr char kENSResolveMethod[] = "brave.ens.resolve_method";

inline constexpr char kEnsOffchainResolveMethod[] =
    "brave.ens.offchain_resolve_method";

// Used to determine which method should be used to resolve SNS domains,
// between:
// Disabled: Disable all SNS domains resolution.
// Ask: Ask users if they want to enable support of SNS.
// Enabled: Resolve domain name using Solana JSON-RPC server.
inline constexpr char kSnsResolveMethod[] = "brave.sns.resolve_method";

// Added 12/2023.
inline constexpr char kSnsResolveMethodMigrated[] =
    "brave.sns.resolve_method_migrated";

}  // namespace decentralized_dns

#endif  // BRAVE_COMPONENTS_DECENTRALIZED_DNS_CORE_PREF_NAMES_H_
