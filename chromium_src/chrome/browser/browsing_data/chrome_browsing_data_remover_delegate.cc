/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_host_content_settings_map.h"
#include "components/content_settings/core/browser/content_settings_info.h"
#include "components/content_settings/core/browser/website_settings_info.h"

namespace {
void ClearFlashPluginType(HostContentSettingsMap* map,
                          base::Time begin_time,
                          base::Time end_time) {
  static_cast<BraveHostContentSettingsMap*>(map)->
      ClearSettingsForPluginsType(begin_time, end_time, false /* is_shields */);
}
}  // namespace

// Skip javascript also. Javascript will be handled by shields settings.
// Clear only flash resource among plugins type because other resource are
// shields resources.
#define HANDLE_PLUGINS_AND_JS_TYPE \
  if (info->website_settings_info()->type() == \
      ContentSettingsType::JAVASCRIPT) \
    continue; \
  if (info->website_settings_info()->type() == \
      ContentSettingsType::PLUGINS) { \
    ClearFlashPluginType( \
        host_content_settings_map_, delete_begin_, delete_end_); \
    continue; \
  }

#include "../../../../../chrome/browser/browsing_data/chrome_browsing_data_remover_delegate.cc"
#undef HANDLE_PLUGINS_AND_JS_TYPE
