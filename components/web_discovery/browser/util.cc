/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/util.h"

#include "base/strings/stringprintf.h"
#include "brave/brave_domains/service_domains.h"
#include "third_party/re2/src/re2/re2.h"

namespace web_discovery {

namespace {
constexpr char kCollectorHostPrefix[] = "collector.wdp";
constexpr char kQuorumHostPrefix[] = "quorum.wdp";
constexpr char kPatternsHostPrefix[] = "patterns.wdp";
constexpr char kPatternsPath[] = "/patterns.gz";
constexpr char kNotAlphanumericRegex[] = "[^A-Za-z0-9]";
}  // namespace

std::string GetCollectorHost() {
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

void TransformToAlphanumeric(std::string& value) {
  re2::RE2 cleaning_regex(kNotAlphanumericRegex);
  re2::RE2::GlobalReplace(&value, cleaning_regex, "");
}

}  // namespace web_discovery
