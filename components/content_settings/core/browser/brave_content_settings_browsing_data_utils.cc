
/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_browsing_data_utils.h"

#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/browser/permission_settings_registry.h"
#include "components/content_settings/core/browser/website_settings_registry.h"

namespace browsing_data {

void BraveRemoveSiteSettingsData(
    const base::Time& delete_begin,
    const base::Time& delete_end,
    HostContentSettingsMap* host_content_settings_map) {
#if BUILDFLAG(IS_IOS)
  // Remove content settings (shields enabled/disabled, block scripts,
  // fingerprinting). These are removed upstream in `browsing_data_helper.cc`
  // for other platforms but not iOS.
  const auto* permission_settings_registry =
      content_settings::PermissionSettingsRegistry::GetInstance();
  for (const content_settings::PermissionSettingsInfo* info :
       *permission_settings_registry) {
    host_content_settings_map->ClearSettingsForOneTypeWithPredicate(
        info->website_settings_info()->type(), delete_begin, delete_end,
        HostContentSettingsMap::PatternSourcePredicate());
  }
#endif

  static constexpr ContentSettingsType kBraveWebSettings[] = {
      ContentSettingsType::BRAVE_COSMETIC_FILTERING,
      ContentSettingsType::BRAVE_AUTO_SHRED, ContentSettingsType::BRAVE_PSST};

  for (const auto type : kBraveWebSettings) {
    if (!content_settings::WebsiteSettingsRegistry::GetInstance()->Get(type)) {
      // If setting isn't registered for some reason (g.e. feature is disabled)
      // then skip it.
      continue;
    }

    host_content_settings_map->ClearSettingsForOneTypeWithPredicate(
        type, delete_begin, delete_end,
        HostContentSettingsMap::PatternSourcePredicate());
  }
}

}  // namespace browsing_data
