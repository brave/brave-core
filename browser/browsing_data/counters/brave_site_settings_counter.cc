/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/browsing_data/counters/brave_site_settings_counter.h"

#include <set>
#include <string>

#include "brave/components/content_settings/core/browser/brave_content_settings_pref_provider.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"
#include "components/browsing_data/core/pref_names.h"
#include "components/content_settings/core/browser/content_settings_registry.h"
#include "components/content_settings/core/common/content_settings_pattern.h"

BraveSiteSettingsCounter::BraveSiteSettingsCounter(
    HostContentSettingsMap* map,
    content::HostZoomMap* zoom_map,
    ProtocolHandlerRegistry* handler_registry,
    PrefService* pref_service)
    : SiteSettingsCounter(map, zoom_map, handler_registry, pref_service),
      map_(map) {}

BraveSiteSettingsCounter::~BraveSiteSettingsCounter() = default;

int BraveSiteSettingsCounter::CountShieldsSettings() {
  std::set<std::string> hosts;
  int empty_host_pattern = 0;
  base::Time period_start = GetPeriodStart();
  base::Time period_end = GetPeriodEnd();

  auto iterate_content_settings_list =
      [&](ContentSettingsType content_type,
          const ContentSettingsForOneType& content_settings_list) {
        for (const auto& content_setting : content_settings_list) {
          // We don't care other source except preference because all shields
          // settings are stored in pref storage.
          if (content_setting.source != "preference")
            continue;

          base::Time last_modified;
          DCHECK(content_settings::IsShieldsContentSettingsType(content_type));
          // Fetching last time for specific resource ids.
          last_modified =
              map_->GetPrefProvider()->GetWebsiteSettingLastModified(
                  content_setting.primary_pattern,
                  content_setting.secondary_pattern,
                  content_type);
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
    ContentSettingsForOneType content_settings_list;
    if (content_settings::IsShieldsContentSettingsType(type)) {
      for (const auto& content_type : content_settings::GetShieldsContentSettingsTypes()) {
        map_->GetSettingsForOneType(content_type, &content_settings_list);
        iterate_content_settings_list(type, content_settings_list);
      }
      continue;
    }
  }

  return hosts.size() + empty_host_pattern;
}

void BraveSiteSettingsCounter::ReportResult(ResultInt value) {
  SiteSettingsCounter::ReportResult(value + CountShieldsSettings());
}
