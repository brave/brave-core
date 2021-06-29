/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/cookies/cookie_options.h"

#include "net/cookies/cookie_access_delegate.h"

#define CookieOptions CookieOptions_ChromiumImpl
#include "../../../../net/cookies/cookie_options.cc"
#undef CookieOptions

namespace net {

CookieOptions::CookieOptions() = default;
CookieOptions::CookieOptions(const CookieOptions&) = default;
CookieOptions::CookieOptions(CookieOptions&&) = default;
CookieOptions::~CookieOptions() = default;
CookieOptions& CookieOptions::operator=(const CookieOptions&) = default;
CookieOptions& CookieOptions::operator=(CookieOptions&&) = default;

CookieOptions::CookieOptions(const CookieOptions_ChromiumImpl& rhs)
    : CookieOptions_ChromiumImpl(rhs) {}
CookieOptions::CookieOptions(CookieOptions_ChromiumImpl&& rhs)
    : CookieOptions_ChromiumImpl(std::move(rhs)) {}

void FillEphemeralStorageParams(
    const GURL& url,
    const SiteForCookies& site_for_cookies,
    const base::Optional<url::Origin>& top_frame_origin,
    const CookieAccessDelegate* cookie_access_delegate,
    CookieOptions* cookie_options) {
  DCHECK(cookie_options);
  if (!cookie_access_delegate) {
    return;
  }
  cookie_options->set_should_use_ephemeral_storage(
      cookie_access_delegate->ShouldUseEphemeralStorage(url, site_for_cookies,
                                                        top_frame_origin));
  if (cookie_options->should_use_ephemeral_storage()) {
    cookie_options->set_site_for_cookies(site_for_cookies);
    cookie_options->set_top_frame_origin(top_frame_origin);
  }
}

}  // namespace net
