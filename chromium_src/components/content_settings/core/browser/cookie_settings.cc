/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/content_settings/core/browser/cookie_settings.h"

#include "net/base/features.h"
#include "net/base/url_util.h"

#define BRAVE_COOKIE_SETTINGS_GET_COOKIES_SETTINGS_INTERNAL               \
  if (!block && is_third_party_request) {                                 \
    block = ShouldBlockThirdPartyIfSettingIsExplicit(                     \
        ShouldBlockThirdPartyCookies(), setting, IsExplicitSetting(info), \
        first_party_url.SchemeIs(extension_scheme_));                     \
  }                                                                       \
  if (auto* setting_with_brave_metadata =                                 \
          cookie_setting_with_brave_metadata()) {                         \
    setting_with_brave_metadata->primary_pattern_matches_all_hosts =      \
        info.primary_pattern.MatchesAllHosts();                           \
    setting_with_brave_metadata->secondary_pattern_matches_all_hosts =    \
        info.secondary_pattern.MatchesAllHosts();                         \
  }

#define ShutdownOnUIThread ShutdownOnUIThread_ChromiumImpl

#include "../../../../../../components/content_settings/core/browser/cookie_settings.cc"

#undef ShutdownOnUIThread
#undef BRAVE_COOKIE_SETTINGS_GET_COOKIES_SETTINGS_INTERNAL

namespace content_settings {

void CookieSettings::ShutdownOnUIThread() {
  ShutdownOnUIThread_ChromiumImpl();
  ephemeral_storage_origins_.clear();
}

bool CookieSettings::ShouldUseEphemeralStorage(
    const url::Origin& origin,
    const net::SiteForCookies& site_for_cookies,
    const absl::optional<url::Origin>& top_frame_origin,
    url::Origin& storage_origin) {
  const bool should_use = CookieSettingsBase::ShouldUseEphemeralStorage(
      origin.GetURL(), site_for_cookies, top_frame_origin);
  if (!should_use) {
    return false;
  }
  DCHECK(top_frame_origin);
  const std::string ephemeral_storage_domain =
      net::URLToEphemeralStorageDomain(top_frame_origin->GetURL());

  auto ephemeral_storage_origins_it =
      ephemeral_storage_origins_.find(ephemeral_storage_domain);
  if (ephemeral_storage_origins_it != ephemeral_storage_origins_.end()) {
    const auto& storage_origins = ephemeral_storage_origins_it->second;
    auto storage_origin_it = storage_origins.find(origin);
    if (storage_origin_it != storage_origins.end()) {
      storage_origin = storage_origin_it->second;
      return true;
    }
  } else {
    ephemeral_storage_origins_it =
        ephemeral_storage_origins_
            .emplace(ephemeral_storage_domain,
                     EphemeralStorageOrigins::mapped_type())
            .first;
  }

  url::Origin opaque_origin = origin.DeriveNewOpaqueOrigin();
  ephemeral_storage_origins_it->second[origin] = opaque_origin;
  storage_origin = std::move(opaque_origin);
  return true;
}

std::vector<url::Origin> CookieSettings::TakeEphemeralStorageOpaqueOrigins(
    const std::string& ephemeral_storage_domain) {
  std::vector<url::Origin> result;
  auto ephemeral_storage_origins_it =
      ephemeral_storage_origins_.find(ephemeral_storage_domain);
  if (ephemeral_storage_origins_it != ephemeral_storage_origins_.end()) {
    result.reserve(ephemeral_storage_origins_it->second.size());
    for (auto& origins : ephemeral_storage_origins_it->second) {
      result.push_back(std::move(origins.second));
    }
    ephemeral_storage_origins_.erase(ephemeral_storage_origins_it);
  }
  return result;
}

}  // namespace content_settings
