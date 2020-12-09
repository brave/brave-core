/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_ephemeral_provider.h"

#include <memory>
#include <utility>

#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"

namespace content_settings {

BraveEphemeralProvider::BraveEphemeralProvider(bool store_last_modified)
    : EphemeralProvider(store_last_modified) {}

bool BraveEphemeralProvider::SetWebsiteSetting(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    std::unique_ptr<base::Value>&& in_value,
    const ContentSettingConstraints& constraint) {
  // Prevent this handle shields configuration.
  if (IsShieldsContentSettingsType(content_type)) {
    return false;
  }

  return EphemeralProvider::SetWebsiteSetting(
      primary_pattern, secondary_pattern, content_type, std::move(in_value),
      constraint);
}

}  // namespace content_settings
