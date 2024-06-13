/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PAYLOAD_GENERATOR_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PAYLOAD_GENERATOR_H_

#include <memory>
#include <vector>

#include "base/values.h"
#include "brave/components/web_discovery/browser/content_scraper.h"
#include "brave/components/web_discovery/browser/patterns.h"
#include "brave/components/web_discovery/browser/regex_util.h"

namespace web_discovery {

inline constexpr char kActionKey[] = "action";
inline constexpr char kInnerPayloadKey[] = "payload";

std::vector<base::Value::Dict> GeneratePayloads(
    const ServerConfig& server_config,
    RegexUtil& regex_util,
    const PatternsURLDetails* url_details,
    std::unique_ptr<PageScrapeResult> scrape_result);

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PAYLOAD_GENERATOR_H_
