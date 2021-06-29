/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_RESTRICTED_COOKIE_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_RESTRICTED_COOKIE_MANAGER_H_

#include "net/base/isolation_info.h"
#include "net/cookies/cookie_access_delegate.h"
#include "net/cookies/cookie_options.h"
#include "net/cookies/site_for_cookies.h"
#include "services/network/public/mojom/restricted_cookie_manager.mojom.h"

#define RemoveChangeListener                                          \
  NotUsed() const {}                                                  \
  net::CookieOptions MakeOptionsForSet(                               \
      mojom::RestrictedCookieManagerRole role, const GURL& url,       \
      const net::SiteForCookies& site_for_cookies,                    \
      const net::IsolationInfo& isolation_info,                       \
      const CookieSettings* cookie_settings,                          \
      const net::CookieAccessDelegate* cookie_access_delegate) const; \
  net::CookieOptions MakeOptionsForGet(                               \
      mojom::RestrictedCookieManagerRole role, const GURL& url,       \
      const net::SiteForCookies& site_for_cookies,                    \
      const net::IsolationInfo& isolation_info,                       \
      const CookieSettings* cookie_settings,                          \
      const net::CookieAccessDelegate* cookie_access_delegate) const; \
  void RemoveChangeListener

#include "../../../../services/network/restricted_cookie_manager.h"

#undef RemoveChangeListener

#endif  // BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_RESTRICTED_COOKIE_MANAGER_H_
