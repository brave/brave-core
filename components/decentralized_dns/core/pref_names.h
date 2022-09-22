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
// Ethereum: Resolve domain name using Ethereum JSON-RPC server.
constexpr char kUnstoppableDomainsResolveMethod[] =
    "brave.unstoppable_domains.resolve_method";

// Used to determine which method should be used to resolve ENS domains,
// between:
// Disabled: Disable all ENS domains resolution.
// Ask: Ask users if they want to enable support of ENS.
// Ethereum: Resolve domain name using Ethereum JSON-RPC server.
constexpr char kENSResolveMethod[] = "brave.ens.resolve_method";

constexpr char kEnsOffchainResolveMethod[] =
    "brave.ens.offchain_resolve_method";

}  // namespace decentralized_dns

#endif  // BRAVE_COMPONENTS_DECENTRALIZED_DNS_CORE_PREF_NAMES_H_
