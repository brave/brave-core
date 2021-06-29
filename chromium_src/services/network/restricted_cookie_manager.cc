/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "services/network/restricted_cookie_manager.h"

#include "base/feature_list.h"
#include "components/content_settings/core/common/cookie_settings_base.h"
#include "net/base/features.h"
#include "net/cookies/cookie_monster.h"
#include "net/cookies/site_for_cookies.h"
#include "services/network/cookie_settings.h"
#include "url/gurl.h"
#include "url/origin.h"

#define IsCookieAccessAllowed IsEphemeralCookieAccessAllowed
#include "../../../../services/network/restricted_cookie_manager.cc"
#undef IsCookieAccessAllowed

namespace network {

net::CookieOptions RestrictedCookieManager::MakeOptionsForSet(
    mojom::RestrictedCookieManagerRole role,
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const net::IsolationInfo& isolation_info,
    const CookieSettings* cookie_settings,
    const net::CookieAccessDelegate* cookie_access_delegate) const {
  net::CookieOptions cookie_options =
      ::network::MakeOptionsForSet(role, url, site_for_cookies, isolation_info,
                                   cookie_settings, cookie_access_delegate);
  net::FillEphemeralStorageParams(url, site_for_cookies, BoundTopFrameOrigin(),
                                  cookie_access_delegate, &cookie_options);
  return cookie_options;
}

net::CookieOptions RestrictedCookieManager::MakeOptionsForGet(
    mojom::RestrictedCookieManagerRole role,
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const net::IsolationInfo& isolation_info,
    const CookieSettings* cookie_settings,
    const net::CookieAccessDelegate* cookie_access_delegate) const {
  net::CookieOptions cookie_options =
      ::network::MakeOptionsForGet(role, url, site_for_cookies, isolation_info,
                                   cookie_settings, cookie_access_delegate);
  net::FillEphemeralStorageParams(url, site_for_cookies, BoundTopFrameOrigin(),
                                  cookie_access_delegate, &cookie_options);
  return cookie_options;
}

}  // namespace network
