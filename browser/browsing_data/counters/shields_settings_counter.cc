/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/browsing_data/counters/shields_settings_counter.h"

#include <set>
#include <string>

#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"
#include "brave/components/content_settings/core/browser/brave_host_content_settings_map.h"
#include "components/browsing_data/core/pref_names.h"
#include "components/content_settings/core/browser/content_settings_registry.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_pattern.h"

ShieldsSettingsCounter::ShieldsSettingsCounter(HostContentSettingsMap* map)
    : map_(map) {
  DCHECK(map_);
}

ShieldsSettingsCounter::~ShieldsSettingsCounter() = default;

void ShieldsSettingsCounter::OnInitialized() {}

const char* ShieldsSettingsCounter::GetPrefName() const {
  return browsing_data::prefs::kDeleteShieldsSettings;
}

void ShieldsSettingsCounter::Count() {
  // Accumulate shields and Javascript site settings counts.
  std::set<std::string> hosts;
  int empty_host_pattern = 0;
  base::Time period_start = GetPeriodStart();
  base::Time period_end = GetPeriodEnd();

  auto iterate_content_settings_list =
      [&](ContentSettingsType content_type,
          const ContentSettingsForOneType& content_settings_list,
          const std::string& resource_identifier) {
        for (const auto& content_setting : content_settings_list) {
          // We don't care other source except preference because all shields
          // settings are stored in pref storage.
          if (content_setting.source != "preference")
            continue;

          base::Time last_modified;
          if (content_type == ContentSettingsType::PLUGINS) {
            // Fetching last time for specific resource ids.
            last_modified = static_cast<BraveHostContentSettingsMap*>(
                map_.get())->GetShieldsSettingLastModifiedDate(
                    content_setting.primary_pattern,
                    content_setting.secondary_pattern,
                    resource_identifier);
          } else {
            DCHECK_EQ(ContentSettingsType::JAVASCRIPT, content_type);
            last_modified = map_->GetSettingLastModifiedDate(
                content_setting.primary_pattern,
                content_setting.secondary_pattern, content_type);
          }
          if (last_modified >= period_start && last_modified < period_end) {
            if (content_setting.primary_pattern.GetHost().empty())
              empty_host_pattern++;
            else
              hosts.insert(content_setting.primary_pattern.GetHost());
          }
        }
      };

  auto* registry = content_settings::ContentSettingsRegistry::GetInstance();
  for (const content_settings::ContentSettingsInfo* info : *registry) {
    ContentSettingsType type = info->website_settings_info()->type();
    // Only care about plugins and javascript type for shields.
    if (type != ContentSettingsType::PLUGINS &&
        type != ContentSettingsType::JAVASCRIPT)
      continue;

    ContentSettingsForOneType content_settings_list;
    if (type == ContentSettingsType::JAVASCRIPT) {
      map_->GetSettingsForOneType(type, content_settings::ResourceIdentifier(),
                                  &content_settings_list);
      iterate_content_settings_list(type, content_settings_list, std::string());
      continue;
    }

    if (type == ContentSettingsType::PLUGINS) {
      for (const auto& id : content_settings::GetShieldsResourceIDs()) {
        map_->GetSettingsForOneType(type, id, &content_settings_list);
        iterate_content_settings_list(type, content_settings_list, id);
      }
      continue;
    }
  }

  ReportResult(hosts.size() + empty_host_pattern);
}
