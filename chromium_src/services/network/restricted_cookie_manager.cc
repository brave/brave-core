/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "services/network/restricted_cookie_manager.h"

#include "net/cookies/cookie_options.h"
#include "services/network/cookie_settings.h"
#include "url/gurl.h"
#include "url/origin.h"

#include <services/network/restricted_cookie_manager.cc>

namespace network {

net::CookieOptions RestrictedCookieManager::MakeOptionsForSet(
    mojom::RestrictedCookieManagerRole role,
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const url::Origin& top_frame_origin,
    const CookieSettings& cookie_settings) const {
  net::CookieOptions cookie_options = ::network::MakeOptionsForSet(
      role, url, site_for_cookies, top_frame_origin, cookie_settings);
  net::FillEphemeralStorageParams(url, site_for_cookies, top_frame_origin,
                                  cookie_store_->cookie_access_delegate(),
                                  &cookie_options);
  return cookie_options;
}

net::CookieOptions RestrictedCookieManager::MakeOptionsForGet(
    mojom::RestrictedCookieManagerRole role,
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const url::Origin& top_frame_origin,
    const CookieSettings& cookie_settings) const {
  net::CookieOptions cookie_options = ::network::MakeOptionsForGet(
      role, url, site_for_cookies, top_frame_origin, cookie_settings);
  net::FillEphemeralStorageParams(url, site_for_cookies, top_frame_origin,
                                  cookie_store_->cookie_access_delegate(),
                                  &cookie_options);
  return cookie_options;
}

}  // namespace network
