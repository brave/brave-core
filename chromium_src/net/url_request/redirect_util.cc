/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/url_request/redirect_util.h"

#include "base/stl_util.h"
#include "net/url_request/url_request_job.h"

#define UpdateHttpRequest UpdateHttpRequest_ChromiumImpl
#include "../../../../net/url_request/redirect_util.cc"
#undef UpdateHttpRequest

namespace net {

void RedirectUtil::UpdateHttpRequest(
    const GURL& original_url,
    const std::string& original_method,
    const RedirectInfo& redirect_info,
    const absl::optional<std::vector<std::string>>& removed_headers,
    const absl::optional<net::HttpRequestHeaders>& modified_headers,
    HttpRequestHeaders* request_headers,
    bool* should_clear_upload) {
  UpdateHttpRequest_ChromiumImpl(original_url,
                                 original_method,
                                 redirect_info,
                                 removed_headers,
                                 modified_headers,
                                 request_headers,
                                 should_clear_upload);
  // Hack for capping referrers at the network layer.
  if (removed_headers) {
    if (base::Contains(*removed_headers, "X-Brave-Cap-Referrer")) {
      GURL capped_referrer = URLRequestJob::ComputeReferrerForPolicy(
          ReferrerPolicy::REDUCE_GRANULARITY_ON_TRANSITION_CROSS_ORIGIN,
          GURL(redirect_info.new_referrer), redirect_info.new_url);
      const_cast<RedirectInfo&>(redirect_info).new_referrer =
          capped_referrer.spec();
    }
  }
}

}  // namespace net
