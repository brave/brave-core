/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_UTIL_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_UTIL_H_

#include <memory>
#include <string>

#include "base/time/time.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/gurl.h"

namespace web_discovery {

inline constexpr size_t kMaxResponseSize = 16 * 1024;
inline constexpr char kCollectorHostSwitch[] = "wdp-collector-host";
inline constexpr int kCurrentVersion = 1;

std::string GetCollectorHost();
GURL GetPatternsEndpoint();

std::unique_ptr<network::ResourceRequest> CreateResourceRequest(GURL url);

std::string FormatServerDate(const base::Time& date);

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_UTIL_H_
