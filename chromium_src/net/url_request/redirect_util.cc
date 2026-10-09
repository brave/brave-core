/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/url_request/redirect_util.h"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "net/url_request/redirect_info.h"
#include "net/url_request/url_request_job.h"

namespace net {

namespace {

// Hack for capping referrers at the network layer.
void MaybeCapReferrer(
    const std::optional<std::vector<std::string>>& removed_headers,
    const RedirectInfo& redirect_info) {
  if (removed_headers &&
      std::ranges::contains(*removed_headers, "X-Brave-Cap-Referrer")) {
    GURL capped_referrer = URLRequestJob::ComputeReferrerForPolicy(
        ReferrerPolicy::REDUCE_GRANULARITY_ON_TRANSITION_CROSS_ORIGIN,
        GURL(redirect_info.new_referrer), redirect_info.new_url);
    const_cast<RedirectInfo&>(redirect_info).new_referrer =
        capped_referrer.spec();
  }
}

}  // namespace

}  // namespace net

#include <net/url_request/redirect_util.cc>
