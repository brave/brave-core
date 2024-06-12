/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_UTIL_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_UTIL_H_

#include <memory>
#include <optional>
#include <string>

#include "base/time/time.h"
#include "net/base/backoff_entry.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/gurl.h"

namespace web_discovery {

inline constexpr size_t kMaxResponseSize = 16 * 1024;
inline constexpr char kCollectorHostSwitch[] = "wdp-collector-host";
inline constexpr char kVersionHeader[] = "Version";
inline constexpr int kCurrentVersion = 1;

inline constexpr net::BackoffEntry::Policy kBackoffPolicy = {
    .num_errors_to_ignore = 0,
    .initial_delay_ms = 10 * 1000,
    .multiply_factor = 2.0,
    .jitter_factor = 0.1,
    .maximum_backoff_ms = 10 * 60 * 1000,
    .entry_lifetime_ms = -1,
    .always_use_initial_delay = false};

std::string GetCollectorHost();
std::string GetQuorumHost();
GURL GetPatternsEndpoint();

std::unique_ptr<network::ResourceRequest> CreateResourceRequest(GURL url);

std::string FormatServerDate(const base::Time& date);

void TransformToAlphanumeric(std::string& value);

std::string DecodeURLComponent(const std::string_view value);

std::optional<std::string> ExtractValueFromQueryString(
    const std::string_view query_string,
    const std::string_view key);

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_UTIL_H_
