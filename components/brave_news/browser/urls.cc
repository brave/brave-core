// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/urls.h"

#include <string>

#include "base/logging.h"
#include "base/strings/strcat.h"
#include "brave/brave_domains/service_domains.h"

namespace brave_news {

namespace {
const char kBraveNewsHostnamePrefix[] = "brave-today-cdn";
const char kPCDNHostnamePrefix[] = "pcdn";
}  // namespace

std::string GetHostname() {
  return brave_domains::GetServicesDomain(kBraveNewsHostnamePrefix);
}

std::string GetMatchingPCDNHostname() {
  // Check for expected PCDN hostname in the feed returned from the server
  // that the feed files are fetched from (brave-today-cdn.xyz)
  std::string feed_hostname = GetHostname();
  if (!feed_hostname.starts_with(kBraveNewsHostnamePrefix)) {
    // Format has changed, return something that otherwise makes sense
    DLOG(ERROR) << "Feed hostname \"" << feed_hostname
                << "\" unexpectedly did not start with prefix \""
                << kBraveNewsHostnamePrefix << "\"";
    return brave_domains::GetServicesDomain(kPCDNHostnamePrefix);
  }
  auto base = feed_hostname.substr(strlen(kBraveNewsHostnamePrefix));
  return base::StrCat({kPCDNHostnamePrefix, base});
}

}  // namespace brave_news
