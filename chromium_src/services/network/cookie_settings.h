/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_COOKIE_SETTINGS_H_
#define BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_COOKIE_SETTINGS_H_

#define CreateDeleteCookieOnExitPredicate                             \
  Unused();                                                           \
  bool IsEphemeralCookieAccessible(                                   \
      const net::CanonicalCookie& cookie, const GURL& url,            \
      const net::SiteForCookies& site_for_cookies,                    \
      const absl::optional<url::Origin>& top_frame_origin,            \
      net::CookieSettingOverrides overrides,                          \
      net::CookieInclusionStatus* cookie_inclusion_status) const;     \
  net::NetworkDelegate::PrivacySetting IsEphemeralPrivacyModeEnabled( \
      const GURL& url, const net::SiteForCookies& site_for_cookies,   \
      const absl::optional<url::Origin>& top_frame_origin,            \
      net::CookieSettingOverrides overrides) const;                   \
  bool AnnotateAndMoveUserBlockedEphemeralCookies(                    \
      const GURL& url, const net::SiteForCookies& site_for_cookies,   \
      const url::Origin* top_frame_origin,                            \
      const net::FirstPartySetMetadata& first_party_set_metadata,     \
      net::CookieSettingOverrides overrides,                          \
      net::CookieAccessResultList& maybe_included_cookies,            \
      net::CookieAccessResultList& excluded_cookies) const;           \
  DeleteCookiePredicate CreateDeleteCookieOnExitPredicate

#include "src/services/network/cookie_settings.h"  // IWYU pragma: export

#undef CreateDeleteCookieOnExitPredicate

#endif  // BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_COOKIE_SETTINGS_H_
