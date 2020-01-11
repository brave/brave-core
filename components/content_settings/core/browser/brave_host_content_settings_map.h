/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_HOST_CONTENT_SETTINGS_MAP_H_
#define BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_HOST_CONTENT_SETTINGS_MAP_H_

#include "components/content_settings/core/browser/host_content_settings_map.h"

class BraveHostContentSettingsMap : public HostContentSettingsMap {
 public:
  using HostContentSettingsMap::HostContentSettingsMap;

  BraveHostContentSettingsMap(const BraveHostContentSettingsMap&) = delete;
  BraveHostContentSettingsMap& operator=(
      const BraveHostContentSettingsMap&) = delete;

  // Clear shields or flash type content settings with time constraint.
  // If |shields| is true clear shields type. Otherwise, clear flash type.
  void ClearSettingsForPluginsType(base::Time begin_time,
                                   base::Time end_time,
                                   bool is_shields);

 private:
  ~BraveHostContentSettingsMap() override = default;

  // Clear all settings if |shields| is true. Otherwise, clear flash type.
  void ClearSettingsForPluginsType(bool shields);

  content_settings::UserModifiableProvider* GetEphemeralProvider();
};

#endif  // BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_HOST_CONTENT_SETTINGS_MAP_H_
