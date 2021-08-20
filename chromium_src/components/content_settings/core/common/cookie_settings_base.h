/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_COOKIE_SETTINGS_BASE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_COOKIE_SETTINGS_BASE_H_

#include "base/auto_reset.h"
#include "components/content_settings/core/common/content_settings.h"

namespace content_settings {

// Helper to allow patchless ephemeral storage access in Chromium code.
using ScopedEphemeralStorageAwareness = base::AutoReset<bool>;

// Contains some useful settings metadata which is not accessible via default
// accessors.
struct CookieSettingWithBraveMetadata {
  CookieSettingWithBraveMetadata();
  CookieSettingWithBraveMetadata(const CookieSettingWithBraveMetadata&);
  CookieSettingWithBraveMetadata(CookieSettingWithBraveMetadata&&);
  CookieSettingWithBraveMetadata& operator=(
      const CookieSettingWithBraveMetadata&);
  CookieSettingWithBraveMetadata& operator=(CookieSettingWithBraveMetadata&&);
  ~CookieSettingWithBraveMetadata();

  ContentSetting setting = CONTENT_SETTING_DEFAULT;
  bool primary_pattern_matches_all_hosts = false;
  bool secondary_pattern_matches_all_hosts = false;
};

}  // namespace content_settings

#define IsCookieSessionOnly                                                    \
  ShouldUseEphemeralStorage(                                                   \
      const GURL& url, const net::SiteForCookies& site_for_cookies,            \
      const absl::optional<url::Origin>& top_frame_origin) const;              \
  ScopedEphemeralStorageAwareness CreateScopedEphemeralStorageAwareness()      \
      const;                                                                   \
  bool IsEphemeralCookieAccessAllowed(const GURL& url,                         \
                                      const GURL& first_party_url) const;      \
  bool IsEphemeralCookieAccessAllowed(                                         \
      const GURL& url, const net::SiteForCookies& site_for_cookies,            \
      const absl::optional<url::Origin>& top_frame_origin) const;              \
  bool IsChromiumFullCookieAccessAllowed(const GURL& url,                      \
                                         const GURL& first_party_url) const;   \
  bool IsChromiumFullCookieAccessAllowed(                                      \
      const GURL& url, const net::SiteForCookies& site_for_cookies,            \
      const absl::optional<url::Origin>& top_frame_origin) const;              \
  bool ShouldBlockThirdPartyIfSettingIsExplicit(                               \
      bool block_third_party_cookies, ContentSetting cookie_setting,           \
      bool is_explicit_setting, bool is_first_party_allowed_scheme) const;     \
  CookieSettingWithBraveMetadata GetCookieSettingWithBraveMetadata(            \
      const GURL& url, const GURL& first_party_url) const;                     \
  CookieSettingWithBraveMetadata* cookie_setting_with_brave_metadata() const { \
    return cookie_setting_with_brave_metadata_;                                \
  }                                                                            \
                                                                               \
 private:                                                                      \
  bool IsCookieAccessAllowedImpl(                                              \
      const GURL& url, const net::SiteForCookies& site_for_cookies,            \
      const absl::optional<url::Origin>& top_frame_origin) const;              \
                                                                               \
  mutable bool ephemeral_storage_aware_ = false;                               \
  mutable CookieSettingWithBraveMetadata*                                      \
      cookie_setting_with_brave_metadata_ = nullptr;                           \
                                                                               \
 public:                                                                       \
  bool IsCookieSessionOnly

#include "../../../../../../components/content_settings/core/common/cookie_settings_base.h"

#undef IsCookieSessionOnly

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_COOKIE_SETTINGS_BASE_H_
