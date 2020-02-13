/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_host_content_settings_map.h"
#include "components/content_settings/core/browser/content_settings_info.h"
#include "components/content_settings/core/browser/website_settings_info.h"

#define CLEAR_SHIELDS_SETTINGS \
  static_cast<BraveHostContentSettingsMap*>(host_content_settings_map_)-> \
      ClearSettingsForPluginsType(delete_begin_, delete_end_);
#include "../../../../../chrome/browser/browsing_data/chrome_browsing_data_remover_delegate.cc"
#undef CLEAR_SHIELDS_SETTINGS
