/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/url_request/url_request_http_job.h"

#include "../../../../net/url_request/url_request_http_job.cc"

namespace net {

CookieOptions URLRequestHttpJob::CreateCookieOptions(
    CookieOptions::SameSiteCookieContext same_site_context,
    CookieOptions::SamePartyCookieContextType same_party_context,
    const IsolationInfo& isolation_info,
    bool is_in_nontrivial_first_party_set) const {
  CookieOptions cookie_options =
      ::CreateCookieOptions(same_site_context, same_party_context,
                            isolation_info, is_in_nontrivial_first_party_set);
  FillEphemeralStorageParams(
      request_->url(), request_->site_for_cookies(),
      isolation_info.top_frame_origin(),
      request_->context()->cookie_store()->cookie_access_delegate(),
      &cookie_options);
  return cookie_options;
}

}  // namespace net
