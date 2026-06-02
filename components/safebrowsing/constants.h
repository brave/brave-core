/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SAFEBROWSING_CONSTANTS_H_
#define BRAVE_COMPONENTS_SAFEBROWSING_CONSTANTS_H_

namespace safe_browsing {

inline constexpr char kBraveSafeBrowsingDownloadProtectionEnabled[] =
    "brave.safebrowsing.download_protection_enabled";

// Sentinel value used by Brave's "Limited protection" Safe Browsing mode.
// Picked above upstream's SafeBrowsingState range (0/1/2) so upstream's
// out-of-range validation keeps rejecting unknown values and future Chromium
// additions don't collide.
inline constexpr int kBraveSafeBrowsingLimitedProtection = 10;

}  // namespace safe_browsing

#endif  // BRAVE_COMPONENTS_SAFEBROWSING_CONSTANTS_H_
