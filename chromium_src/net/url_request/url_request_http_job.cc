/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/url_request/url_request_http_job.h"

#include "net/http/transport_security_state.h"

namespace net {

namespace {
// The Accept-Language header position is different when farbling is "off" vs
// when farbling is "on". This class helps to keep the position same to remove
// farbling detection by sites.
// See https://github.com/brave/brave-browser/issues/55271 for more details.
class BraveAcceptLanguageHttpHeaderReposition {
 public:
  explicit BraveAcceptLanguageHttpHeaderReposition(
      HttpRequestHeaders& headers) {
    // We try and move the position of the Accept-Language header to the end.
    auto modified_language_value =
        headers.GetHeader(HttpRequestHeaders::kAcceptLanguage);
    if (modified_language_value.has_value()) {
      // We remove the header first from the underlying vector.
      headers.RemoveHeader(HttpRequestHeaders::kAcceptLanguage);
      // This will put the header at the end.
      headers.SetHeader(HttpRequestHeaders::kAcceptLanguage,
                        modified_language_value.value());
    }
  }

  ~BraveAcceptLanguageHttpHeaderReposition() = default;
};
}  // namespace
}  // namespace net

#define GetSSLUpgradeDecision(host, is_top_level_nav, net_log)                 \
  GetSSLUpgradeDecision(request->isolation_info().network_anonymization_key(), \
                        host, is_top_level_nav, net_log)
#define ShouldSSLErrorsBeFatal(host) \
  ShouldSSLErrorsBeFatal(            \
      request_->isolation_info().network_anonymization_key(), host)
#define AddHSTSHeader(host, value) \
  AddHSTSHeader(request_->isolation_info(), host, value)

#define BRAVE_URL_REQUEST_HTTP_JOB_ADD_EXTRA_HEADERS \
  BraveAcceptLanguageHttpHeaderReposition(request_info_.extra_headers);

#include <net/url_request/url_request_http_job.cc>

#undef BRAVE_URL_REQUEST_HTTP_JOB_ADD_EXTRA_HEADERS
#undef AddHSTSHeader
#undef ShouldSSLErrorsBeFatal
#undef GetSSLUpgradeDecision

namespace net {

namespace {

// This function acts as a trampoline between
// `URLRequestHttpJob::CreateCookieOptions` and the `CreateCookieOptions`
// declared in the annonymous namespace inside `net::`. This is necessary
// because otherwise there's no way for `URLRequestHttpJob::CreateCookieOptions`
// to capture calls to `CreateCookieOptions` and at the same time be able to
// call it in the annonymous namespace.
CookieOptions CreateCookieOptionsCaller(
    CookieOptions::SameSiteCookieContext same_site_context) {
  return CreateCookieOptions(same_site_context);
}

}  // namespace

CookieOptions URLRequestHttpJob::CreateCookieOptions(
    CookieOptions::SameSiteCookieContext same_site_context) const {
  CookieOptions cookie_options = CreateCookieOptionsCaller(same_site_context);
  FillEphemeralStorageParams(
      request_->url(), request_->site_for_cookies(),
      request_->isolation_info().top_frame_origin(),
      request_->context()->cookie_store()->cookie_access_delegate(),
      &cookie_options);
  return cookie_options;
}

}  // namespace net
