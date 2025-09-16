/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_CONSTANTS_H_

#include <string_view>

namespace brave_ads {

// TODO(tmancey): Discuss cnames with Jacob.

inline constexpr std::string_view kStagingKeyConfigUrl =
    "https://ads-serve.bravesoftware.com/v1/ohttp/hpkekeyconfig";
inline constexpr std::string_view kStagingRelayUrl =
    "https://brave-ohttp-relay-dev.fastly-edge.com/v1/ohttp/gateway";

inline constexpr std::string_view kProductionKeyConfigUrl =
    "https://ads-serve.brave.com/v1/ohttp/hpkekeyconfig";
inline constexpr std::string_view kProductionRelayUrl =
    "https://brave-ohttp-relay-prod.fastly-edge.com/v1/ohttp/gateway";

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_CONSTANTS_H_
