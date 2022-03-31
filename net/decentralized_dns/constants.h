/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_NET_DECENTRALIZED_DNS_CONSTANTS_H_
#define BRAVE_NET_DECENTRALIZED_DNS_CONSTANTS_H_

namespace decentralized_dns {

constexpr char kCryptoDomain[] = ".crypto";
constexpr char kUnstoppableDomainsDoHResolver[] =
    "https://resolver.unstoppable.io/dns-query{?brave_UD}";

constexpr char kEthDomain[] = ".eth";
constexpr char kDNSForEthDomain[] = ".eth.link";
constexpr char kENSDoHResolver[] =
    "https://resolver.cloudflare-eth.com/dns-query{?brave_ENS}";

}  // namespace decentralized_dns

#endif  // BRAVE_NET_DECENTRALIZED_DNS_CONSTANTS_H_
