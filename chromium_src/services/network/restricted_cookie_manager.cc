/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "services/network/restricted_cookie_manager.h"

#include <utility>

#include "base/feature_list.h"
#include "components/content_settings/core/common/cookie_settings_base.h"
#include "net/base/features.h"
#include "net/cookies/cookie_monster.h"
#include "net/cookies/site_for_cookies.h"
#include "net/storage_access_api/status.h"
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
#define GetCookiesString GetCookiesString_ChromiumImpl

#include <services/network/restricted_cookie_manager.cc>

#undef GetCookiesString

#undef IsFullCookieAccessAllowed
#undef AnnotateAndMoveUserBlockedCookies
#undef IsCookieAccessible

namespace network {

void RestrictedCookieManager::GetCookiesString(
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const url::Origin& top_frame_origin,
    net::StorageAccessApiStatus storage_access_api_status,
    bool get_version_shared_memory,
    bool is_ad_tagged,
    bool apply_devtools_overrides,
    bool force_disable_third_party_cookies,
    GetCookiesStringCallback callback) {
  // Kill switch: skip shared-memory versioning so CookieJar stays on the
  // sync IPC path.
  if (!base::FeatureList::IsEnabled(
          net::features::kBraveCookieJarSharedVersion)) {
    get_version_shared_memory = false;
  }
  GetCookiesString_ChromiumImpl(
      url, site_for_cookies, top_frame_origin, storage_access_api_status,
      get_version_shared_memory, is_ad_tagged, apply_devtools_overrides,
      force_disable_third_party_cookies, std::move(callback));
}

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
