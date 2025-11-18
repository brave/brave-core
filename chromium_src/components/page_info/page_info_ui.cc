/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Add JAVASCRIPT_OPTIMIZER to the permission UI info list so it can be
// displayed in Page Info with proper localized strings.
#define BRAVE_GET_CONTENT_SETTINGS_UI_INFO      \
  {ContentSettingsType::JAVASCRIPT_OPTIMIZER,   \
   IDS_SITE_SETTINGS_TYPE_JAVASCRIPT_OPTIMIZER, \
   IDS_SITE_SETTINGS_TYPE_JAVASCRIPT_OPTIMIZER},

#include <components/page_info/page_info_ui.cc>

#undef BRAVE_GET_CONTENT_SETTINGS_UI_INFO
