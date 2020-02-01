/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BROWSING_DATA_COUNTERS_SHIELDS_SETTINGS_COUNTER_H_
#define BRAVE_BROWSER_BROWSING_DATA_COUNTERS_SHIELDS_SETTINGS_COUNTER_H_

#include "components/browsing_data/core/counters/browsing_data_counter.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"

class ShieldsSettingsCounter : public browsing_data::BrowsingDataCounter {
 public:
  explicit ShieldsSettingsCounter(HostContentSettingsMap* map);
  ~ShieldsSettingsCounter() override;

  const char* GetPrefName() const override;

 private:
  void OnInitialized() override;

  void Count() override;

  scoped_refptr<HostContentSettingsMap> map_;
};

#endif  // BRAVE_BROWSER_BROWSING_DATA_COUNTERS_SHIELDS_SETTINGS_COUNTER_H_
