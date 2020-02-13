/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BROWSING_DATA_COUNTERS_BRAVE_SITE_SETTINGS_COUNTER_H_
#define BRAVE_BROWSER_BROWSING_DATA_COUNTERS_BRAVE_SITE_SETTINGS_COUNTER_H_

#include "brave/components/content_settings/core/browser/brave_host_content_settings_map.h"
#include "chrome/browser/browsing_data/counters/site_settings_counter.h"

// This class adds shieldss settings count
class BraveSiteSettingsCounter : public SiteSettingsCounter {
 public:
  BraveSiteSettingsCounter(HostContentSettingsMap* map,
                           content::HostZoomMap* zoom_map,
                           ProtocolHandlerRegistry* handler_registry,
                           PrefService* pref_service);
  ~BraveSiteSettingsCounter() override;
  BraveSiteSettingsCounter(const BraveSiteSettingsCounter&) = delete;
  BraveSiteSettingsCounter& operator=(const BraveSiteSettingsCounter&) = delete;

 private:
  // SiteSettingsCounter overrides:
  void ReportResult(ResultInt value) override;

  int CountShieldsSettings();

  scoped_refptr<BraveHostContentSettingsMap> map_;
};

#endif  // BRAVE_BROWSER_BROWSING_DATA_COUNTERS_BRAVE_SITE_SETTINGS_COUNTER_H_
