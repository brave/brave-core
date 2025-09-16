/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_CONSTANTS_H_

#include <string_view>

namespace brave_ads {

inline constexpr std::string_view kStagingRelayUrl =
    "https://brave-ohttp-relay-dev.fastly-edge.com/v1/ohttp/gateway";
inline constexpr std::string_view kProductionRelayUrl =
    "https://brave-ohttp-relay-prod.fastly-edge.com/v1/ohttp/gateway";

// TODO(tmancey): Remove this constant before merging and use a proper key
// management system.
inline constexpr char kHpkeKeyHex[] =
    "0000204d2b5332acfa391e6fb004d8f916631b2b5155f248202911ccaf5539441f524c"
    "000400010002";

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_CONSTANTS_H_
