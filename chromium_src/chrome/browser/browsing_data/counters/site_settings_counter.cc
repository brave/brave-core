/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/browsing_data/counters/site_settings_counter.h"

#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"

namespace {

int GetBraveShieldsDefaultsCount(base::Time period_start,
                                 base::Time period_end,
                                 HostContentSettingsMap* map) {
  int empty_host_pattern = 0;
  auto iterate_content_settings_list =
      [&](ContentSettingsType content_type,
          const ContentSettingsForOneType& content_settings_list) {
        for (const auto& content_setting : content_settings_list) {
          if (content_settings::GetSettingSourceFromProviderType(
                  content_setting.source) ==
                  content_settings::SettingSource::kUser &&
              content_setting.source !=
                  content_settings::ProviderType::kDefaultProvider) {
            base::Time last_modified = content_setting.metadata.last_modified();
            if (last_modified >= period_start && last_modified < period_end) {
              if (content_setting.primary_pattern.GetHost().empty()) {
                empty_host_pattern++;
              }
            }
          }
        }
      };

  for (const auto type : content_settings::GetShieldsContentSettingsTypes()) {
    iterate_content_settings_list(type, map->GetSettingsForOneType(type));
  }

  return empty_host_pattern;
}

}  // namespace

#define ReportResult(...)                                  \
  ReportResult(__VA_ARGS__ - GetBraveShieldsDefaultsCount( \
                                 period_start, period_end, map_.get()))

#include "src/chrome/browser/browsing_data/counters/site_settings_counter.cc"

#undef ReportResult
