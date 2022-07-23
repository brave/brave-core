/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_util.h"

#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "url/gurl.h"

namespace speedreader {

bool PageSupportsDistillation(DistillState state) {
  return state == DistillState::kSpeedreaderOnDisabledPage ||
         state == DistillState::kPageProbablyReadable;
}

bool PageStateIsDistilled(DistillState state) {
  return state == DistillState::kReaderMode ||
         state == DistillState::kSpeedreaderMode;
}

bool PageWantsDistill(DistillState state) {
  return state == DistillState::kReaderMode ||
         state == DistillState::kSpeedreaderMode ||
         state == DistillState::kReaderModePending ||
         state == DistillState::kSpeedreaderModePending;
}

void SetEnabledForSite(HostContentSettingsMap* map,
                       const GURL& url,
                       bool enable) {
  DCHECK(!url.is_empty());  // Not supported. Disable Speedreader in settings.

  // Rule covers all protocols and pages.
  auto pattern = ContentSettingsPattern::FromString("*://" + url.host() + "/*");
  if (!pattern.IsValid())
    return;

  ContentSetting setting =
      enable ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK;
  map->SetContentSettingCustomScope(pattern, ContentSettingsPattern::Wildcard(),
                                    ContentSettingsType::BRAVE_SPEEDREADER,
                                    setting);
}

bool IsEnabledForSite(HostContentSettingsMap* map, const GURL& url) {
  ContentSetting setting = map->GetContentSetting(
      url, GURL(), ContentSettingsType::BRAVE_SPEEDREADER);
  const bool enabled =
      setting == CONTENT_SETTING_ALLOW || setting == CONTENT_SETTING_DEFAULT;
  return enabled;
}

}  // namespace speedreader
