/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_COOKIE_SETTINGS_H_
#define BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_COOKIE_SETTINGS_H_

#define IsCookieAccessible                                        \
  IsEphemeralCookieAccessible(                                    \
      const net::CanonicalCookie& cookie, const GURL& url,        \
      const GURL& site_for_cookies,                               \
      const absl::optional<url::Origin>& top_frame_origin) const; \
  bool IsCookieAccessible

#include "../../../../../../services/network/cookie_settings.h"

#undef IsCookieAccessible

#endif  // BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_COOKIE_SETTINGS_H_
