/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_RESTRICTED_COOKIE_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_RESTRICTED_COOKIE_MANAGER_H_

#include "net/base/isolation_info.h"
#include "net/cookies/cookie_access_delegate.h"
#include "net/cookies/cookie_options.h"
#include "net/cookies/site_for_cookies.h"
#include "services/network/public/mojom/restricted_cookie_manager.mojom.h"

#define RemoveChangeListener                                    \
  NotUsed() const {}                                            \
  net::CookieOptions MakeOptionsForSet(                         \
      mojom::RestrictedCookieManagerRole role, const GURL& url, \
      const net::SiteForCookies& site_for_cookies,              \
      const url::Origin& top_frame_origin,                      \
      const CookieSettings& cookie_settings) const;             \
  net::CookieOptions MakeOptionsForGet(                         \
      mojom::RestrictedCookieManagerRole role, const GURL& url, \
      const net::SiteForCookies& site_for_cookies,              \
      const url::Origin& top_frame_origin,                      \
      const CookieSettings& cookie_settings) const;             \
  void RemoveChangeListener

#define GetCookiesString                                                     \
  GetCookiesString_ChromiumImpl(                                             \
      const GURL& url, const net::SiteForCookies& site_for_cookies,          \
      const url::Origin& top_frame_origin,                                   \
      net::StorageAccessApiStatus storage_access_api_status,                 \
      bool get_version_shared_memory, bool is_ad_tagged,                     \
      bool apply_devtools_overrides, bool force_disable_third_party_cookies, \
      GetCookiesStringCallback callback);                                    \
  void GetCookiesString

#include <services/network/restricted_cookie_manager.h>  // IWYU pragma: export

#undef GetCookiesString
#undef RemoveChangeListener

#endif  // BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_RESTRICTED_COOKIE_MANAGER_H_
