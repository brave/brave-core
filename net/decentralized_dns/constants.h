/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_NET_DECENTRALIZED_DNS_CONSTANTS_H_
#define BRAVE_NET_DECENTRALIZED_DNS_CONSTANTS_H_

namespace decentralized_dns {

inline constexpr const char* kUnstoppableDomains[] = {
    ".crypto",     ".x",          ".nft",     ".dao",         ".wallet",
    ".blockchain", ".bitcoin",    ".zil",     ".altimist",    ".anime",
    ".klever",     ".manga",      ".polygon", ".unstoppable", ".pudgy",
    ".tball",      ".stepn",      ".secret",  ".raiin",       ".pog",
    ".clay",       ".metropolis", ".witg",    ".ubu",         ".kryptic",
    ".farms",      ".dfz"};

inline constexpr char kEthDomain[] = ".eth";
inline constexpr char kDNSForEthDomain[] = ".eth.link";

inline constexpr char kSolDomain[] = ".sol";

}  // namespace decentralized_dns

#endif  // BRAVE_NET_DECENTRALIZED_DNS_CONSTANTS_H_
