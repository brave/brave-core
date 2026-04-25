/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/browsing_data/content/browsing_data_helper.h"

#include "brave/components/content_settings/core/browser/brave_content_settings_browsing_data_utils.h"

#define RemoveSiteSettingsData RemoveSiteSettingsData_ChromiumImpl

#include <components/browsing_data/content/browsing_data_helper.cc>

#undef RemoveSiteSettingsData

namespace browsing_data {

void RemoveSiteSettingsData(const base::Time& delete_begin,
                            const base::Time& delete_end,
                            HostContentSettingsMap* host_content_settings_map) {
  RemoveSiteSettingsData_ChromiumImpl(delete_begin, delete_end,
                                      host_content_settings_map);

  BraveRemoveSiteSettingsData(delete_begin, delete_end,
                              host_content_settings_map);
}

}  // namespace browsing_data
