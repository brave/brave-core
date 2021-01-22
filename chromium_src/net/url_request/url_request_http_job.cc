/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/url_request/url_request_http_job.h"

#include "base/bind.h"
#include "net/base/features.h"
#include "net/base/isolation_info.h"
#include "net/cookies/cookie_monster.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"

namespace {

bool ShouldUseEphemeralStorage(net::URLRequest* request) {
  if (!base::FeatureList::IsEnabled(net::features::kBraveEphemeralStorage))
    return false;

  const net::IsolationInfo& isolation_info = request->isolation_info();
  if (!isolation_info.top_frame_origin().has_value() ||
      !isolation_info.frame_origin().has_value())
    return false;
  if (*isolation_info.top_frame_origin() == *isolation_info.frame_origin())
    return false;

  if (url::Origin::Create(request->url()) == *isolation_info.top_frame_origin())
    return false;

  auto top_frame_url = isolation_info.top_frame_origin()->GetURL();
  auto top_request = request->context()->CreateRequest(
      top_frame_url, request->priority(), nullptr);
  top_request->set_site_for_cookies(
      net::SiteForCookies::FromUrl(top_frame_url));
  auto options = net::CookieOptions();
  if (!(request->network_delegate()->CanSetCookie(
            *top_request, net::CanonicalCookie(), &options, true) &&
        !request->network_delegate()->CanSetCookie(
            *request, net::CanonicalCookie(), &options, true)))
    return false;

  return true;
}

}  // namespace

#define BRAVE_ADDCOOKIEHEADERANDSTART                                          \
  if (ShouldUseEphemeralStorage(request_)) {                                   \
    static_cast<CookieMonster*>(cookie_store)                                  \
        ->GetEphemeralCookieListWithOptionsAsync(                              \
            request_->url(),                                                   \
            request()->isolation_info().top_frame_origin()->GetURL(), options, \
            base::BindOnce(&URLRequestHttpJob::SetCookieHeaderAndStart,        \
                           weak_factory_.GetWeakPtr(), options));              \
    return;                                                                    \
  }

#define BRAVE_SAVECOOKIESANDNOTIFYHEADERSCOMPLETE                              \
  if (ShouldUseEphemeralStorage(request_)) {                                   \
    static_cast<CookieMonster*>(request_->context()->cookie_store())           \
        ->SetEphemeralCanonicalCookieAsync(                                    \
            std::move(cookie), request_->url(),                                \
            request()->isolation_info().top_frame_origin()->GetURL(), options, \
            base::BindOnce(&URLRequestHttpJob::OnSetCookieResult,              \
                           weak_factory_.GetWeakPtr(), options,                \
                           cookie_to_return, cookie_string));                  \
    return;                                                                    \
  }

#define CanGetCookies() \
  (ShouldUseEphemeralStorage(request_) ? true : CanGetCookies())
#include "../../../../../net/url_request/url_request_http_job.cc"
#undef CanGetCookies
