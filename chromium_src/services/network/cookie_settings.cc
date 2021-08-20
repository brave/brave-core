/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "services/network/cookie_settings.h"

#include "net/base/features.h"
#include "url/origin.h"

#define BRAVE_COOKIE_SETTINGS_GET_COOKIE_SETTINGS_INTERNAL                     \
  if (!blocked_by_third_party_setting && is_third_party_request) {             \
    blocked_by_third_party_setting = ShouldBlockThirdPartyIfSettingIsExplicit( \
        block_third_party_cookies_, cookie_setting,                            \
        content_settings::IsExplicitSetting(*entry),                           \
        base::Contains(third_party_cookies_allowed_schemes_,                   \
                       first_party_url.scheme()));                             \
  }                                                                            \
  if (auto* setting_with_brave_metadata =                                      \
          cookie_setting_with_brave_metadata()) {                              \
    setting_with_brave_metadata->primary_pattern_matches_all_hosts =           \
        entry->primary_pattern.MatchesAllHosts();                              \
    setting_with_brave_metadata->secondary_pattern_matches_all_hosts =         \
        entry->secondary_pattern.MatchesAllHosts();                            \
  }

#include "../../../../services/network/cookie_settings.cc"

#undef BRAVE_COOKIE_SETTINGS_GET_COOKIE_SETTINGS_INTERNAL

namespace network {

bool CookieSettings::IsEphemeralCookieAccessible(
    const net::CanonicalCookie& cookie,
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const absl::optional<url::Origin>& top_frame_origin) const {
  // Upstream now do single cookie-specific checks in some places to determine
  // whether cookie access should be granted. However, when ephemeral storage is
  // enabled, Brave doesn't care about whether access is being requested for a
  // specific cookie or not, so we simply return |true| if that's the case.
  // See https://crrev.com/c/2895004 for the upstream change that required this.
  if (IsEphemeralCookieAccessAllowed(url, site_for_cookies, top_frame_origin))
    return true;

  return IsCookieAccessible(cookie, url, site_for_cookies, top_frame_origin);
}

bool CookieSettings::IsEphemeralPrivacyModeEnabled(
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const absl::optional<url::Origin>& top_frame_origin,
    net::SamePartyContext::Type same_party_cookie_context_type) const {
  if (IsEphemeralCookieAccessAllowed(url, site_for_cookies, top_frame_origin))
    return false;

  return IsPrivacyModeEnabled(url, site_for_cookies, top_frame_origin,
                              same_party_cookie_context_type);
}

bool CookieSettings::AnnotateAndMoveUserBlockedEphemeralCookies(
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const url::Origin* top_frame_origin,
    net::CookieAccessResultList& maybe_included_cookies,
    net::CookieAccessResultList& excluded_cookies) const {
  absl::optional<url::Origin> top_frame_origin_opt;
  if (top_frame_origin)
    top_frame_origin_opt = *top_frame_origin;

  if (IsEphemeralCookieAccessAllowed(url, site_for_cookies,
                                     top_frame_origin_opt)) {
    return true;
  }

  return AnnotateAndMoveUserBlockedCookies(
      url, site_for_cookies, top_frame_origin, maybe_included_cookies,
      excluded_cookies);
}

}  // namespace network
