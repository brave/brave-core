/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/util.h"

#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/brave_domains/service_domains.h"
#include "url/url_util.h"

namespace web_discovery {

namespace {
constexpr char kCollectorHostPrefix[] = "collector.wdp";
constexpr char kQuorumHostPrefix[] = "quorum.wdp";
constexpr char kPatternsHostPrefix[] = "patterns.wdp";
constexpr char kPatternsPath[] = "/patterns.gz";
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
  return GURL(base::StrCat(
      {url::kHttpsScheme, url::kStandardSchemeSeparator,
       brave_domains::GetServicesDomain(kPatternsHostPrefix), kPatternsPath}));
}

std::unique_ptr<network::ResourceRequest> CreateResourceRequest(GURL url) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = url;
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  return resource_request;
}

std::string FormatServerDate(const base::Time& date) {
  base::Time::Exploded exploded;
  date.UTCExplode(&exploded);
  return base::StringPrintf("%04d%02d%02d", exploded.year, exploded.month,
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

}  // namespace web_discovery
