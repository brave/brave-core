/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_NETWORK_AUDIT_ALLOWED_LISTS_H_
#define BRAVE_BROWSER_NET_BRAVE_NETWORK_AUDIT_ALLOWED_LISTS_H_

namespace brave {

// Before adding to this list, get approval from the security team.
inline constexpr const char* kAllowedUrlProtocols[] = {
    "chrome-extension", "chrome", "brave", "file", "data", "blob",
};

// Before adding to this list, get approval from the security team.
inline constexpr const char* kAllowedUrlPrefixes[] = {
    // allowed because it 307's to https://componentupdater.brave.com
    "https://componentupdater.brave.com/service/update2",
    "https://crxdownload.brave.com/crx/blobs/",

    // Omaha/Sparkle
    "https://updates.bravesoftware.com/",

    // stats/referrals
    "https://laptop-updates.brave.com/",
    "https://laptop-updates-staging.brave.com/",

    // needed for DoH on Mac build machines
    "https://dns.google/dns-query",

    // needed for DoH on Mac build machines
    "https://chrome.cloudflare-dns.com/dns-query",

    // for fetching tor client updater component
    "https://tor.bravesoftware.com/",

    // brave sync v2 production
    "https://sync-v2.brave.com/v2",

    // brave sync v2 staging
    "https://sync-v2.bravesoftware.com/v2",

    // brave sync v2 dev
    "https://sync-v2.brave.software/v2",

    // brave A/B testing
    "https://variations.brave.com/seed",

    // Brave News (production)
    "https://brave-today-cdn.brave.com/",

    // Brave's Privacy-focused CDN
    "https://pcdn.brave.com/",

    // Brave Rewards production
    "https://api.rewards.brave.com/v1/parameters",
    "https://rewards.brave.com/publishers/prefix-list",
    "https://grant.rewards.brave.com/v1/promotions",

    // Brave Rewards staging & dev
    "https://api.rewards.bravesoftware.com/v1/parameters",
    "https://rewards-stg.bravesoftware.com/publishers/prefix-list",
    "https://grant.rewards.bravesoftware.com/v1/promotions",

    // p3a
    "https://p3a-creative.brave.com/",
    "https://p3a-json.brave.com/",
    "https://p3a.brave.com/",
    "https://star-randsrv.bsg.brave.com/",

    // Other
    "https://brave-core-ext.s3.brave.com/",
    "https://dict.brave.com/",
    "https://go-updater.brave.com/",
    "https://redirector.brave.com/",
    "https://safebrowsing.brave.com/",
    "https://static.brave.com/",
    "https://static1.brave.com/",
};

// Before adding to this list, get approval from the security team.
inline constexpr const char* kAllowedUrlPatterns[] = {
    // allowed because it's url for fetching super referral's mapping table
    "https://mobile-data.s3.brave.com/superreferrer/map-table.json",
    "https://mobile-data-dev.s3.brave.software/superreferrer/map-table.json",
};

}  // namespace brave

#endif  // BRAVE_BROWSER_NET_BRAVE_NETWORK_AUDIT_ALLOWED_LISTS_H_
