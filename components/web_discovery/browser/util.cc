/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/util.h"

#include "base/strings/stringprintf.h"
#include "brave/brave_domains/service_domains.h"

namespace web_discovery {

namespace {
constexpr char kCollectorHostPrefix[] = "collector.wdp";
constexpr char kVersionHeader[] = "Version";
}  // namespace

std::string GetCollectorHost() {
  auto* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(kCollectorHostSwitch)) {
    return cmd_line->GetSwitchValueASCII(kCollectorHostSwitch);
  }
  return base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                       brave_domains::GetServicesDomain(kCollectorHostPrefix)});
}

std::unique_ptr<network::ResourceRequest> CreateResourceRequest(GURL url) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = url;
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->headers.SetHeader(kVersionHeader,
                                      base::NumberToString(kCurrentVersion));
  return resource_request;
}

std::string FormatServerDate(const base::Time& date) {
  base::Time::Exploded exploded;
  date.UTCExplode(&exploded);
  return base::StringPrintf("%04d%02d%02d", exploded.year, exploded.month,
                            exploded.day_of_month);
}

}  // namespace web_discovery
