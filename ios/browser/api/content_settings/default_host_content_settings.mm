// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/content_settings/default_host_content_settings.h"

#include "base/memory/scoped_refptr.h"
#include "base/values.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "net/base/apple/url_conversions.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

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

- (BOOL)popupsAllowed {
  auto setting = _settingsMap->GetDefaultContentSetting(
      ContentSettingsType::POPUPS, nullptr);
  return setting == CONTENT_SETTING_ALLOW;
}

- (void)setPopupsAllowed:(BOOL)popupsAllowed {
  _settingsMap->SetDefaultContentSetting(
      ContentSettingsType::POPUPS,
      popupsAllowed ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK);
}

- (ExternalLinkMode)defaultExternalLinkMode {
  return [self
      externalLinkModeFromValue:
          _settingsMap->GetWebsiteSetting(
              GURL(), GURL(), ContentSettingsType::BRAVE_EXTERNAL_LINK_MODE)];
}

- (void)setDefaultExternalLinkMode:(ExternalLinkMode)mode {
  _settingsMap->SetWebsiteSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_EXTERNAL_LINK_MODE,
      base::Value(static_cast<int>(mode)));
}

- (ExternalLinkMode)externalLinkModeForURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  if (!gurl.is_valid()) {
    return self.defaultExternalLinkMode;
  }
  return
      [self externalLinkModeFromValue:
                _settingsMap->GetWebsiteSetting(
                    gurl, gurl, ContentSettingsType::BRAVE_EXTERNAL_LINK_MODE)];
}

- (void)setExternalLinkMode:(ExternalLinkMode)mode forURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  if (!gurl.is_valid()) {
    return;
  }
  std::string base_domain =
      net::registry_controlled_domains::GetDomainAndRegistry(
          gurl, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  ContentSettingsPattern pattern =
      base_domain.empty()
          ? ContentSettingsPattern::FromURL(gurl)
          : ContentSettingsPattern::FromString("[*.]" + base_domain);
  _settingsMap->SetWebsiteSettingCustomScope(
      pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_EXTERNAL_LINK_MODE,
      base::Value(static_cast<int>(mode)));
}

- (void)clearAllExternalLinkModes {
  for (const auto& entry : _settingsMap->GetSettingsForOneType(
           ContentSettingsType::BRAVE_EXTERNAL_LINK_MODE)) {
    if (entry.primary_pattern == ContentSettingsPattern::Wildcard() &&
        entry.secondary_pattern == ContentSettingsPattern::Wildcard()) {
      continue;
    }
    _settingsMap->SetWebsiteSettingCustomScope(
        entry.primary_pattern, entry.secondary_pattern,
        ContentSettingsType::BRAVE_EXTERNAL_LINK_MODE, base::Value());
  }
}

- (ExternalLinkMode)externalLinkModeFromValue:(const base::Value&)value {
  if (!value.is_int() || value.GetInt() < ExternalLinkModeCurrent ||
      value.GetInt() > ExternalLinkModeQuickView) {
    return ExternalLinkModeCurrent;
  }
  return static_cast<ExternalLinkMode>(value.GetInt());
}

@end
