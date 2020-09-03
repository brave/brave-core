/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/common/ipfs_utils.h"

#include <vector>

#include "url/gurl.h"
#include "extensions/common/url_pattern.h"

namespace ipfs {

// static
bool IpfsUtils::IsIPFSURL(const GURL& gurl) {
  static std::vector<URLPattern> updater_patterns({
      URLPattern(URLPattern::SCHEME_ALL, "*://*/ipfs/*"),
      URLPattern(URLPattern::SCHEME_ALL, "*://*/ipns/*")
  });
  return std::any_of(
      updater_patterns.begin(), updater_patterns.end(),
      [&gurl](URLPattern pattern) { return pattern.MatchesURL(gurl); });
}

}  // namespace ipfs
