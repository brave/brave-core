/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_NETWORK_AUDIT_WHITELISTS_H_
#define BRAVE_BROWSER_NET_BRAVE_NETWORK_AUDIT_WHITELISTS_H_

#include <string>

namespace brave {

// Before adding to this list, get approval from the security team.
constexpr const char* kWhitelistedUrlProtocols[] = {
    "chrome-extension", "chrome", "brave", "file", "data", "blob",
};

// Before adding to this list, get approval from the security team.
constexpr const char* kWhitelistedUrlPrefixes[] = {
    // allowed because it 307's to https://componentupdater.brave.com
    "http://componentupdater.brave.com/service/update2",
    "https://componentupdater.brave.com/service/update2",
    "https://crlsets.brave.com/",
    "https://crxdownload.brave.com/crx/blobs/",

    // Omaha/Sparkle
    "https://updates.bravesoftware.com/",

    // stats/referrals
    "https://laptop-updates.brave.com/",

    // needed for DoH on Mac build machines
    "https://dns.google/dns-query",
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

    // Brave Today (production)
    "https://brave-today-cdn.brave.com/",

    // Brave's Privacy-focused CDN
    "https://pcdn.brave.com/",

    // allowed because it 307's to go-updater.brave.com. should never actually
    // connect to googleapis.com.
    "http://update.googleapis.com/service/update2",
    "https://update.googleapis.com/service/update2",

    // allowed because it 307's to safebrowsing.brave.com
    "https://safebrowsing.googleapis.com/v4/threatListUpdates",
    "https://clients2.googleusercontent.com/crx/blobs/",

    // allowed because it 307's to redirector.brave.com
    "http://dl.google.com/",
    "https://dl.google.com/",

    // fake gaia URL
    "https://no-thanks.invalid/",

    // Other
    "https://brave-core-ext.s3.brave.com/",
    "https://go-updater.brave.com/",
    "https://p3a.brave.com/",
    "https://redirector.brave.com/",
    "https://safebrowsing.brave.com/",
    "https://static.brave.com/",
    "https://static1.brave.com/",
};

// Before adding to this list, get approval from the security team.
constexpr const char* kWhitelistedUrlPatterns[] = {
    // allowed because it 307's to redirector.brave.com
    "http://[A-Za-z0-9-\\.]+\\.gvt1\\.com/edgedl/release2/.+",
    "https://[A-Za-z0-9-\\.]+\\.gvt1\\.com/edgedl/release2/.+",

    // allowed because it 307's to crlsets.brave.com
    "http://www.google.com/dl/release2/chrome_component/.+crl-set.+",
    "https://www.google.com/dl/release2/chrome_component/.+crl-set.+",
    "http://storage.googleapis.com/update-delta/"
    "hfnkpimlhhgieaddgfemjhofmfblmnib/.+crxd",
    "https://storage.googleapis.com/update-delta/"
    "hfnkpimlhhgieaddgfemjhofmfblmnib/.+crxd",

    // allowed because it's url for fetching super referral's mapping table
    "https://mobile-data.s3.brave.com/superreferrer/map-table.json",
    "https://mobile-data-dev.s3.brave.software/superreferrer/map-table.json",
};

}  // namespace brave

#endif  // BRAVE_BROWSER_NET_BRAVE_NETWORK_AUDIT_WHITELISTS_H_
