/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_COOKIE_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_COOKIE_MANAGER_H_

#include "services/network/public/mojom/cookie_manager.mojom.h"

#define GetCookieList                                                  \
  GetCookieList_ChromiumImpl(const GURL& url,                          \
                             const net::CookieOptions& cookie_options, \
                             GetCookieListCallback callback);          \
  void GetCookieList

#define SetCanonicalCookie                                                  \
  SetCanonicalCookie_ChromiumImpl(const net::CanonicalCookie& cookie,       \
                                  const GURL& source_url,                   \
                                  const net::CookieOptions& cookie_options, \
                                  SetCanonicalCookieCallback callback);     \
  void SetCanonicalCookie

#include "../../../../services/network/cookie_manager.h"

#undef SetCanonicalCookie
#undef GetCookieList

#endif  // BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_COOKIE_MANAGER_H_
