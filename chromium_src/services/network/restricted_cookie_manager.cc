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

// IsEphemeralCookieAccessible and AnnotateAndMoveUserBlockedCookies declared &
// defined via a chromium_src override for network/cookie_settings.{h,cc}.
#define IsCookieAccessible IsEphemeralCookieAccessible
#define AnnotateAndMoveUserBlockedCookies \
  AnnotateAndMoveUserBlockedEphemeralCookies

// IsEphemeralCookieAccessAllowed declared & defined via a chromium_src override
// for components/content_settings/core/common/cookie_settings_base.{h,cc}.
#define IsFullCookieAccessAllowed IsEphemeralCookieAccessAllowed

#include "src/services/network/restricted_cookie_manager.cc"

#undef IsFullCookieAccessAllowed
#undef AnnotateAndMoveUserBlockedCookies
#undef IsCookieAccessible

namespace network {

net::CookieOptions RestrictedCookieManager::MakeOptionsForSet(
    mojom::RestrictedCookieManagerRole role,
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const net::IsolationInfo& isolation_info,
    const CookieSettings& cookie_settings,
    const net::FirstPartySetMetadata& first_party_set_metadata) const {
  net::CookieOptions cookie_options =
      ::network::MakeOptionsForSet(role, url, site_for_cookies, isolation_info,
                                   cookie_settings, first_party_set_metadata);
  net::FillEphemeralStorageParams(url, site_for_cookies, BoundTopFrameOrigin(),
                                  cookie_store_->cookie_access_delegate(),
                                  &cookie_options);
  return cookie_options;
}

net::CookieOptions RestrictedCookieManager::MakeOptionsForGet(
    mojom::RestrictedCookieManagerRole role,
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const net::IsolationInfo& isolation_info,
    const CookieSettings& cookie_settings,
    const net::FirstPartySetMetadata& first_party_set_metadata) const {
  net::CookieOptions cookie_options =
      ::network::MakeOptionsForGet(role, url, site_for_cookies, isolation_info,
                                   cookie_settings, first_party_set_metadata);
  net::FillEphemeralStorageParams(url, site_for_cookies, BoundTopFrameOrigin(),
                                  cookie_store_->cookie_access_delegate(),
                                  &cookie_options);
  return cookie_options;
}

}  // namespace network
