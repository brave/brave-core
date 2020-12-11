/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"

#define BRAVE_EARLY_RETURN_ON_SHIELDS_CONTENT_SETTINGS_TYPE         \
  if (content_settings::IsShieldsContentSettingsType(content_type)) \
    return nullptr;

#include "../../../../../../components/content_settings/core/browser/content_settings_global_value_map.cc"

#undef BRAVE_EARLY_RETURN_ON_SHIELDS_CONTENT_SETTINGS_TYPE
