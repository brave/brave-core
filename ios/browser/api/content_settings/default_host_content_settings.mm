// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/content_settings/default_host_content_settings.h"

#include "base/memory/scoped_refptr.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"

@implementation DefaultHostContentSettings {
  scoped_refptr<HostContentSettingsMap> _settingsMap;
}

- (instancetype)initWithSettingsMap:(HostContentSettingsMap*)settingsMap {
  if ((self = [super init])) {
    _settingsMap = settingsMap;
  }
  return self;
}

- (DefaultPageMode)defaultPageMode {
  auto setting = _settingsMap->GetDefaultContentSetting(
      ContentSettingsType::REQUEST_DESKTOP_SITE, nullptr);
  return setting == CONTENT_SETTING_ALLOW ? DefaultPageModeDesktop
                                          : DefaultPageModeMobile;
}

- (void)setDefaultPageMode:(DefaultPageMode)defaultPageMode {
  _settingsMap->SetDefaultContentSetting(
      ContentSettingsType::REQUEST_DESKTOP_SITE,
      defaultPageMode == DefaultPageModeDesktop ? CONTENT_SETTING_ALLOW
                                                : CONTENT_SETTING_BLOCK);
}

@end
