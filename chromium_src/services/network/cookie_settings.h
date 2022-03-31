/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_COOKIE_SETTINGS_H_
#define BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_COOKIE_SETTINGS_H_

#define IsCookieAccessible                                        \
  IsEphemeralCookieAccessible(                                    \
      const net::CanonicalCookie& cookie, const GURL& url,        \
      const net::SiteForCookies& site_for_cookies,                \
      const absl::optional<url::Origin>& top_frame_origin) const; \
  bool IsCookieAccessible

#define IsPrivacyModeEnabled                                        \
  IsEphemeralPrivacyModeEnabled(                                    \
      const GURL& url, const net::SiteForCookies& site_for_cookies, \
      const absl::optional<url::Origin>& top_frame_origin,          \
      net::SamePartyContext::Type same_party_context_type) const;   \
  net::NetworkDelegate::PrivacySetting IsPrivacyModeEnabled

#define AnnotateAndMoveUserBlockedCookies                           \
  AnnotateAndMoveUserBlockedEphemeralCookies(                       \
      const GURL& url, const net::SiteForCookies& site_for_cookies, \
      const url::Origin* top_frame_origin,                          \
      net::CookieAccessResultList& maybe_included_cookies,          \
      net::CookieAccessResultList& excluded_cookies) const;         \
  bool AnnotateAndMoveUserBlockedCookies

#include "src/services/network/cookie_settings.h"

#undef AnnotateAndMoveUserBlockedCookies
#undef IsPrivacyModeEnabled
#undef IsCookieAccessible

#endif  // BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_COOKIE_SETTINGS_H_
