/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_NET_DECENTRALIZED_DNS_CONSTANTS_H_
#define BRAVE_NET_DECENTRALIZED_DNS_CONSTANTS_H_

#include <optional>
#include <string>
#include <string_view>

#include "net/base/net_export.h"

namespace decentralized_dns {

inline constexpr std::string_view kEthDomain = ".eth";
inline constexpr std::string_view kDNSForEthDomain = ".eth.link";
inline constexpr std::string_view kSolDomain = ".sol";

// Checks if a given host ends with the suffix of an unstoppable domain, e.g.
// foo.crypto. Returns a reference to the domain entry.
NET_EXPORT std::optional<std::string_view> GetUnstoppableDomainSuffix(
    std::string_view host);

// Returns a full list of unstoppable domain suffixes separated by commas.
NET_EXPORT std::string GetUnstoppableDomainSuffixFullList();

}  // namespace decentralized_dns

#endif  // BRAVE_NET_DECENTRALIZED_DNS_CONSTANTS_H_
