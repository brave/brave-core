/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/url_request/redirect_util.h"

#define UpdateHttpRequest UpdateHttpRequest_ChromiumImpl
#include "../../../../net/url_request/redirect_util.cc"
#undef UpdateHttpRequest

namespace net {

void RedirectUtil::UpdateHttpRequest(
    const GURL& original_url,
    const std::string& original_method,
    const RedirectInfo& redirect_info,
    const base::Optional<std::vector<std::string>>& removed_headers,
    const base::Optional<net::HttpRequestHeaders>& modified_headers,
    HttpRequestHeaders* request_headers,
    bool* should_clear_upload) {
  UpdateHttpRequest_ChromiumImpl(original_url,
                                 original_method,
                                 redirect_info,
                                 removed_headers,
                                 modified_headers,
                                 request_headers,
                                 should_clear_upload);
  // Hack for dropping referrer at the network layer.
  if (removed_headers) {
    if (removed_headers->end() != std::find(removed_headers->begin(),
                                            removed_headers->end(),
                                            "X-Brave-Clear-Referer")) {
      const_cast<RedirectInfo&>(redirect_info).new_referrer.clear();
    }
  }
}

}  // namespace net
