/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_OBLIVIOUS_HTTP_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_OBLIVIOUS_HTTP_CONSTANTS_H_

#include <string_view>

namespace brave_ads {

inline constexpr std::string_view kStagingObliviousHttpKeyConfigUrl =
    "https://static.ads.bravesoftware.com/v1/ohttp/hpkekeyconfig";
inline constexpr std::string_view kStagingObliviousHttpRelayUrl =
    "https://ohttp.ads.bravesoftware.com/v1/ohttp/gateway";

inline constexpr std::string_view kProductionObliviousHttpKeyConfigUrl =
    "https://static.ads.brave.com/v1/ohttp/hpkekeyconfig";
inline constexpr std::string_view kProductionObliviousHttpRelayUrl =
    "https://ohttp.ads.brave.com/v1/ohttp/gateway";

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_OBLIVIOUS_HTTP_CONSTANTS_H_
