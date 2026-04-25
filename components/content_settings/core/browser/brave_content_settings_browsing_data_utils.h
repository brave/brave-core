/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_BROWSING_DATA_UTILS_H_
#define BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_BROWSING_DATA_UTILS_H_

namespace base {
class Time;
}

class HostContentSettingsMap;

namespace browsing_data {

// Removes Brave content settings when clearing the site & shields browsing
// data.
void BraveRemoveSiteSettingsData(
    const base::Time& delete_begin,
    const base::Time& delete_end,
    HostContentSettingsMap* host_content_settings_map);

}  // namespace browsing_data

#endif  // BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_BROWSING_DATA_UTILS_H_
