/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_GET_RENDER_CONTENT_SETTING_RULES                                \
  map->GetSettingsForOneType(ContentSettingsType::AUTOPLAY,                   \
                             ResourceIdentifier(), &(rules->autoplay_rules)); \
  map->GetSettingsForOneType(ContentSettingsType::PLUGINS, "fingerprinting",  \
                             &(rules->fingerprinting_rules));                 \
  map->GetSettingsForOneType(ContentSettingsType::PLUGINS, "braveShields",    \
                             &(rules->brave_shields_rules));

#include "../../../../../components/content_settings/core/browser/content_settings_utils.cc"  // NOLINT

#undef BRAVE_BRAVE_GET_RENDER_CONTENT_SETTING_RULES
