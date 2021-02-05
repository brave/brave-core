/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define HasRegisteredGroupName HasRegisteredGroupName_ChromiumImpl
#define ContentSettingsTypeToGroupName \
  ContentSettingsTypeToGroupName_ChromiumImpl

// clang-format off
#define BRAVE_CONTENT_SETTINGS_TYPE_GROUP_NAMES_LIST               \
  {ContentSettingsType::BRAVE_ADS, nullptr},                       \
  {ContentSettingsType::BRAVE_COSMETIC_FILTERING, nullptr},        \
  {ContentSettingsType::BRAVE_TRACKERS, nullptr},                  \
  {ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES, nullptr}, \
  {ContentSettingsType::BRAVE_FINGERPRINTING_V2, nullptr},         \
  {ContentSettingsType::BRAVE_SHIELDS, nullptr},                   \
  {ContentSettingsType::BRAVE_REFERRERS, nullptr},                 \
  {ContentSettingsType::BRAVE_COOKIES, nullptr},
// clang-format on

#define BRAVE_SITE_SETTINGS_HELPER_CONTENT_SETTINGS_TYPE_FROM_GROUP_NAME \
  if (name == "autoplay")                                                \
    return ContentSettingsType::AUTOPLAY;

#include "../../../../../../../chrome/browser/ui/webui/settings/site_settings_helper.cc"

#undef BRAVE_CONTENT_SETTINGS_TYPE_GROUP_NAMES_LIST
#undef BRAVE_SITE_SETTINGS_HELPER_CONTENT_SETTINGS_TYPE_FROM_GROUP_NAME
#undef ContentSettingsTypeToGroupName
#undef HasRegisteredGroupName

namespace site_settings {

bool HasRegisteredGroupName(ContentSettingsType type) {
  if (type == ContentSettingsType::AUTOPLAY)
    return true;
  return HasRegisteredGroupName_ChromiumImpl(type);
}

base::StringPiece ContentSettingsTypeToGroupName(ContentSettingsType type) {
  if (type == ContentSettingsType::AUTOPLAY)
    return "autoplay";
  return ContentSettingsTypeToGroupName_ChromiumImpl(type);
}

}  // namespace site_settings
