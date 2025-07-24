/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_NETWORK_AUDIT_ALLOWED_LISTS_H_
#define BRAVE_BROWSER_NET_BRAVE_NETWORK_AUDIT_ALLOWED_LISTS_H_

#include <array>
#include <string_view>

#include "base/containers/fixed_flat_set.h"

namespace brave {

// Before adding to this list, get approval from the security team.
inline constexpr auto kAllowedUrlProtocols =
    base::MakeFixedFlatSet<std::string_view>({
        "chrome-extension",
        "chrome",
        "brave",
        "file",
        "data",
        "blob",
    });

// Before adding to the lists below, get approval from the security team.

// List of URL prefixes that are used in functionality not controlled via
// admin policies.
inline constexpr auto kBaseAllowedUrlPrefixes = std::to_array<std::string_view>({
    // allowed because it 307's to https://componentupdater.brave.com
    "https://componentupdater.brave.com/service/update2",
    "https://crxdownload.brave.com/crx/blobs/",

    // Omaha/Sparkle
    "https://updates.bravesoftware.com/",

    // needed for DoH on Mac build machines
    "https://dns.google/dns-query",

    // needed for DoH on Mac build machines
    "https://chrome.cloudflare-dns.com/dns-query",

    // brave A/B testing
    "https://variations.brave.com/seed",

    // Brave's Privacy-focused CDN
    "https://pcdn.brave.com/",

    // brave sync v2 production
    "https://sync-v2.brave.com/v2",

    // brave sync v2 staging
    "https://sync-v2.bravesoftware.com/v2",

    // brave sync v2 dev
    "https://sync-v2.brave.software/v2",

    // Other
    "https://brave-core-ext.s3.brave.com/",
    "https://dict.brave.com/",
    "https://go-updater.brave.com/",
    "https://redirector.brave.com/",
    "https://safebrowsing.brave.com/",
    "https://static.brave.com/",
    "https://static1.brave.com/",
});

// List of URL prefixes that are used in functionality controlled via
// admin policies. A test that uses this list should also check URLs against
// the base list above.
inline constexpr auto kOtherAllowedUrlPrefixes = std::to_array<std::string_view>({
    // stats/referrals
    "https://usage-ping.brave.com/",
    "https://usage-ping.bravesoftware.com/",

    // for fetching tor client updater component
    "https://tor.bravesoftware.com/",

    // Brave News (production)
    "https://brave-today-cdn.brave.com/",

    // p3a
    "https://star-randsrv.bsg.brave.com/",
    "https://collector.bsg.brave.com/",

    // web discovery
    "https://collector.wdp.brave.com/",
    "https://quorum.wdp.brave.com/",
    "https://patterns.wdp.brave.com/",
});

}  // namespace brave

#endif  // BRAVE_BROWSER_NET_BRAVE_NETWORK_AUDIT_ALLOWED_LISTS_H_
