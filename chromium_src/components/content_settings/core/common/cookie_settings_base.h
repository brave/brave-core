/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_COOKIE_SETTINGS_BASE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_COOKIE_SETTINGS_BASE_H_

#include <optional>

#include "components/content_settings/core/common/content_settings.h"

#define IsCookieSessionOnly                                                \
  ShouldUseEphemeralStorage(                                               \
      const GURL& url, const net::SiteForCookies& site_for_cookies,        \
      base::optional_ref<const url::Origin> top_frame_origin) const;       \
  bool IsEphemeralCookieAccessAllowed(                                     \
      const GURL& url, const net::SiteForCookies& site_for_cookies,        \
      base::optional_ref<const url::Origin> top_frame_origin,              \
      net::CookieSettingOverrides overrides) const;                        \
  bool IsFullCookieAccessAllowed_ChromiumImpl(                             \
      const GURL& url, const net::SiteForCookies& site_for_cookies,        \
      base::optional_ref<const url::Origin> top_frame_origin,              \
      net::CookieSettingOverrides overrides,                               \
      CookieSettingWithMetadata* cookie_settings = nullptr) const;         \
  bool ShouldBlockThirdPartyIfSettingIsExplicit(                           \
      bool block_third_party_cookies, ContentSetting cookie_setting,       \
      bool is_explicit_setting, bool is_first_party_allowed_scheme) const; \
                                                                           \
 public:                                                                   \
  bool IsCookieSessionOnly

#include "src/components/content_settings/core/common/cookie_settings_base.h"  // IWYU pragma: export

#undef IsCookieSessionOnly

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_COOKIE_SETTINGS_BASE_H_
