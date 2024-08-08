/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/url_request/url_request_http_job.h"

#include "net/http/transport_security_state.h"

#define GetSSLUpgradeDecision(host, net_log)                                   \
  GetSSLUpgradeDecision(request->isolation_info().network_anonymization_key(), \
                        host, net_log)
#define ShouldSSLErrorsBeFatal(host) \
  ShouldSSLErrorsBeFatal(            \
      request_->isolation_info().network_anonymization_key(), host)
#define AddHSTSHeader(host, value) \
  AddHSTSHeader(request_->isolation_info(), host, value)

#include "src/net/url_request/url_request_http_job.cc"

#undef AddHSTSHeader
#undef ShouldSSLErrorsBeFatal
#undef GetSSLUpgradeDecision

namespace net {

CookieOptions URLRequestHttpJob::CreateCookieOptions(
    CookieOptions::SameSiteCookieContext same_site_context) const {
  CookieOptions cookie_options = ::CreateCookieOptions(same_site_context);
  FillEphemeralStorageParams(
      request_->url(), request_->site_for_cookies(),
      request_->isolation_info().top_frame_origin(),
      request_->context()->cookie_store()->cookie_access_delegate(),
      &cookie_options);
  return cookie_options;
}

}  // namespace net
