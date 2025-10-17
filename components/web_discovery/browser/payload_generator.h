/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PAYLOAD_GENERATOR_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PAYLOAD_GENERATOR_H_

#include <memory>
#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/web_discovery/browser/content_scraper.h"
#include "brave/components/web_discovery/browser/patterns.h"
#include "brave/components/web_discovery/browser/patterns_v2.h"

namespace web_discovery {

inline constexpr char kActionKey[] = "action";
inline constexpr char kInnerPayloadKey[] = "payload";

// Generates "query" messages using the payload generation rules
// and scraped data for a given site.
std::vector<base::Value::Dict> GenerateQueryPayloads(
    const ServerConfig& server_config,
    const PatternsURLDetails* url_details,
    std::unique_ptr<PageScrapeResult> scrape_result);

// Generates "query" messages for v2 patterns
std::vector<base::Value::Dict> GenerateQueryPayloadsV2(
    const ServerConfig& server_config,
    const V2PatternsGroup& patterns_group,
    std::unique_ptr<PageScrapeResult> scrape_result);

// Generates an "alive" message to indicate an opted-in
// status to the server.
base::Value::Dict GenerateAlivePayload(const ServerConfig& server_config,
                                       std::string date_hour);

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PAYLOAD_GENERATOR_H_
