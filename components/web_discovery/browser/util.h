/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_UTIL_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_UTIL_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "base/time/time.h"
#include "net/base/backoff_entry.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/gurl.h"

namespace web_discovery {

struct ServerConfig;
struct PageScrapeResult;

inline constexpr size_t kMaxResponseSize = 16 * 1024;
inline constexpr char kCollectorHostSwitch[] = "wdp-collector-host";
inline constexpr char kPatternsURLSwitch[] = "wdp-patterns-url";
inline constexpr char kVersionHeader[] = "Version";
inline constexpr int kCurrentVersion = 1;

// Standard attribute IDs used across pattern versions
inline constexpr char kV1UrlAttrId[] = "url";
inline constexpr char kV2UrlAttrId[] = "qurl";
inline constexpr char kCountryCodeAttrId[] = "ctry";
inline constexpr char kQueryAttrId[] = "q";

// The default backoff policy to use for scheduling retry requests.
inline constexpr net::BackoffEntry::Policy kBackoffPolicy = {
    .num_errors_to_ignore = 0,
    .initial_delay_ms = 10 * 1000,
    .multiply_factor = 2.0,
    .jitter_factor = 0.1,
    .maximum_backoff_ms = 10 * 60 * 1000,
    .entry_lifetime_ms = -1,
    .always_use_initial_delay = false};

// Returns the non-proxied HPN host, used for acquiring anonymous credentials.
std::string GetDirectHPNHost();
// Returns the proxied HPN host, used for retrieving server config and page
// content submission.
std::string GetAnonymousHPNHost();
// Returns the "quorum" host, used for location config and page event
// submission.
std::string GetQuorumHost();
// Returns the full URL for the patterns config.
GURL GetPatternsEndpoint();

// Creates a new ResourceRequest with the given URL and credentials omitted.
std::unique_ptr<network::ResourceRequest> CreateResourceRequest(GURL url);

// Formats a given date as a string in the format "YYYYMMDD", in the UTC
// timezone.
std::string FormatServerDate(const base::Time& date);

// Decodes URL-encoded components, converting escape sequences to their
// corresponding characters.
std::string DecodeURLComponent(const std::string_view value);

// Extracts the value associated with a given key from a URL query string.
std::optional<std::string> ExtractValueFromQueryString(
    const std::string_view query_string,
    const std::string_view key);

// Strips non-alphanumeric characters from string
void TransformToAlphanumeric(std::string& str);

// Gets standard request values for common attribute IDs
std::optional<std::string> GetRequestValue(
    std::string_view attr_id,
    const GURL& url,
    const ServerConfig& server_config,
    const PageScrapeResult& scrape_result);

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_UTIL_H_
