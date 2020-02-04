/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define HasRegisteredGroupName HasRegisteredGroupName_ChromiumImpl
#define ContentSettingsTypeFromGroupName \
  ContentSettingsTypeFromGroupName_ChromiumImpl
#define ContentSettingsTypeToGroupName \
  ContentSettingsTypeToGroupName_ChromiumImpl

#include "../../../../../../chrome/browser/ui/webui/site_settings_helper.cc"
#undef ContentSettingsTypeToGroupName
#undef ContentSettingsTypeFromGroupName
#undef HasRegisteredGroupName

namespace site_settings {

bool HasRegisteredGroupName(ContentSettingsType type) {
  if (type == ContentSettingsType::AUTOPLAY)
    return true;
  return HasRegisteredGroupName_ChromiumImpl(type);
}

ContentSettingsType ContentSettingsTypeFromGroupName(const std::string& name) {
  if (name == "autoplay")
    return ContentSettingsType::AUTOPLAY;
  return ContentSettingsTypeFromGroupName_ChromiumImpl(name);
}

std::string ContentSettingsTypeToGroupName(ContentSettingsType type) {
  if (type == ContentSettingsType::AUTOPLAY)
    return "autoplay";
  return ContentSettingsTypeToGroupName_ChromiumImpl(type);
}

}  // namespace site_settings
