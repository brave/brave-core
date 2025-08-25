/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/util.h"

#include <utility>

#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/components/web_discovery/browser/content_scraper.h"
#include "brave/components/web_discovery/browser/server_config_loader.h"
#include "brave/components/web_discovery/common/features.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/url_util.h"

namespace web_discovery {

namespace {
constexpr char kCollectorHostPrefix[] = "collector.wdp";
constexpr char kQuorumHostPrefix[] = "quorum.wdp";
constexpr char kPatternsHostPrefix[] = "patterns.wdp";
constexpr char kPatternsPath[] = "patterns.gz";
constexpr char kV2PatternsPath[] = "patterns-v2.gz";
}  // namespace

std::string GetDirectHPNHost() {
  // TODO(djandries): Replace with non-proxied endpoint once available
  return GetAnonymousHPNHost();
}

std::string GetAnonymousHPNHost() {
  auto* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(kCollectorHostSwitch)) {
    return cmd_line->GetSwitchValueASCII(kCollectorHostSwitch);
  }
  return base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                       brave_domains::GetServicesDomain(kCollectorHostPrefix)});
}

std::string GetQuorumHost() {
  return base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                       brave_domains::GetServicesDomain(kQuorumHostPrefix)});
}

GURL GetPatternsEndpoint() {
  auto* cmd_line = base::CommandLine::ForCurrentProcess();
  std::string patterns_url_str;
  if (cmd_line->HasSwitch(kPatternsURLSwitch)) {
    patterns_url_str = cmd_line->GetSwitchValueASCII(kPatternsURLSwitch);
  } else {
    auto patterns_path = features::kPatternsPath.Get();
    if (patterns_path.empty()) {
      patterns_path =
          features::ShouldUseV2Patterns() ? kV2PatternsPath : kPatternsPath;
    }
    patterns_url_str =
        base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                      brave_domains::GetServicesDomain(kPatternsHostPrefix),
                      "/", patterns_path});
  }

  return GURL(patterns_url_str);
}

std::unique_ptr<network::ResourceRequest> CreateResourceRequest(GURL url) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = std::move(url);
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  return resource_request;
}

std::string FormatServerDate(const base::Time& date) {
  base::Time::Exploded exploded;
  date.UTCExplode(&exploded);
  return absl::StrFormat("%04d%02d%02d", exploded.year, exploded.month,
                         exploded.day_of_month);
}

std::string DecodeURLComponent(const std::string_view value) {
  url::RawCanonOutputT<char16_t> result;
  url::DecodeURLEscapeSequences(value, url::DecodeURLMode::kUTF8OrIsomorphic,
                                &result);
  return base::UTF16ToUTF8(result.view());
}

std::optional<std::string> ExtractValueFromQueryString(
    const std::string_view query_string,
    const std::string_view key) {
  url::Component query_slice(0, query_string.length());
  url::Component key_slice;
  url::Component value_slice;
  while (url::ExtractQueryKeyValue(query_string, &query_slice, &key_slice,
                                   &value_slice)) {
    if (query_string.substr(key_slice.begin, key_slice.len) != key) {
      continue;
    }
    return DecodeURLComponent(
        query_string.substr(value_slice.begin, value_slice.len));
  }
  return std::nullopt;
}

void TransformToAlphanumeric(std::string& str) {
  std::erase_if(str, [](char c) { return !std::isalnum(c); });
}

std::optional<std::string> GetRequestValue(
    std::string_view attr_id,
    const GURL& url,
    const ServerConfig& server_config,
    const PageScrapeResult& scrape_result) {
  if (attr_id == kV1UrlAttrId || attr_id == kV2UrlAttrId) {
    return url.spec();
  } else if (attr_id == kCountryCodeAttrId) {
    return server_config.location;
  } else if (attr_id == kQueryAttrId) {
    auto result = scrape_result.query;
    if (result) {
      base::ReplaceSubstringsAfterOffset(&result.value(), 0, "%20", " ");
    }
    return result;
  }
  return std::nullopt;
}

}  // namespace web_discovery
