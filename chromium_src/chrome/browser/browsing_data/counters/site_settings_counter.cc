/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/browsing_data/counters/site_settings_counter.h"

#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"
#include "components/content_settings/core/browser/permission_settings_registry.h"

namespace {

template <typename Iterator>
void ProcessBraveTypes(Iterator&& iterate_content_settings_list,
                       HostContentSettingsMap* map,
                       int& empty_host_pattern) {
  auto* registry = content_settings::PermissionSettingsRegistry::GetInstance();

  auto fix_empty_host_pattern = [&](ContentSettingsType type) {
    const int saved_empty_host_pattern = empty_host_pattern;
    iterate_content_settings_list(type, map->GetSettingsForOneType(type));
    if (registry->Get(type)) {
      // upstream already counted this type, but empty pattern contains default
      // value, decrease count if it is changed.
      if (empty_host_pattern != saved_empty_host_pattern) {
        empty_host_pattern -=
            2 * (empty_host_pattern - saved_empty_host_pattern);
      }
    } else {
      empty_host_pattern = saved_empty_host_pattern;
    }
  };

  for (const auto type : content_settings::GetShieldsContentSettingsTypes()) {
    fix_empty_host_pattern(type);
  }
  // This is required because we override COOKIES with BRAVE_COOKIES and it's
  // counted twice.
  fix_empty_host_pattern(ContentSettingsType::BRAVE_COOKIES);
}

}  // namespace

#define ReportResult(...)                                      \
  ProcessBraveTypes(iterate_content_settings_list, map_.get(), \
                    empty_host_pattern);                       \
  ReportResult(__VA_ARGS__);

#include <chrome/browser/browsing_data/counters/site_settings_counter.cc>

#undef ReportResult
