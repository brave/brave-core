// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/content_settings/default_host_content_settings.h"

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "brave/components/external_links/core/browser/external_link_handler.h"
#include "brave/components/external_links/core/common/external_link_mode.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

using external_links::ExternalLinkHandler;

static_assert(
    ExternalLinkModeCurrent ==
    static_cast<NSInteger>(external_links::ExternalLinkMode::kCurrent));
static_assert(ExternalLinkModeAsk ==
              static_cast<NSInteger>(external_links::ExternalLinkMode::kAsk));
static_assert(
    ExternalLinkModeRegular ==
    static_cast<NSInteger>(external_links::ExternalLinkMode::kRegular));
static_assert(
    ExternalLinkModePrivateBrowsing ==
    static_cast<NSInteger>(external_links::ExternalLinkMode::kPrivateBrowsing));
static_assert(ExternalLinkModeModal ==
              static_cast<NSInteger>(external_links::ExternalLinkMode::kModal));

static_assert(
    ExternalLinkActionOpenCurrent ==
    static_cast<NSInteger>(external_links::ExternalLinkAction::kOpenCurrent));
static_assert(ExternalLinkActionShowOnboarding ==
              static_cast<NSInteger>(
                  external_links::ExternalLinkAction::kShowOnboarding));
static_assert(
    ExternalLinkActionShowPrompt ==
    static_cast<NSInteger>(external_links::ExternalLinkAction::kShowPrompt));
static_assert(
    ExternalLinkActionOpenRegular ==
    static_cast<NSInteger>(external_links::ExternalLinkAction::kOpenRegular));
static_assert(
    ExternalLinkActionOpenPrivate ==
    static_cast<NSInteger>(external_links::ExternalLinkAction::kOpenPrivate));
static_assert(
    ExternalLinkActionOpenModal ==
    static_cast<NSInteger>(external_links::ExternalLinkAction::kOpenModal));

@implementation DefaultHostContentSettings {
  scoped_refptr<HostContentSettingsMap> _settingsMap;
  std::unique_ptr<ExternalLinkHandler> _externalLinkHandler;
}

- (instancetype)initWithSettingsMap:(HostContentSettingsMap*)settingsMap
                              prefs:(PrefService*)prefs {
  if ((self = [super init])) {
    _settingsMap = settingsMap;
    _externalLinkHandler =
        std::make_unique<ExternalLinkHandler>(settingsMap, prefs);
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
  return static_cast<ExternalLinkMode>(_externalLinkHandler->GetDefaultMode());
}

- (void)setDefaultExternalLinkMode:(ExternalLinkMode)mode {
  _externalLinkHandler->SetDefaultMode(
      static_cast<external_links::ExternalLinkMode>(mode));
}

- (ExternalLinkMode)externalLinkModeForURL:(NSURL*)url {
  return static_cast<ExternalLinkMode>(
      _externalLinkHandler->GetMode(net::GURLWithNSURL(url)));
}

- (void)setExternalLinkMode:(ExternalLinkMode)mode forURL:(NSURL*)url {
  _externalLinkHandler->SetSiteMode(
      net::GURLWithNSURL(url),
      static_cast<external_links::ExternalLinkMode>(mode));
}

- (void)clearAllExternalLinkModes {
  _externalLinkHandler->ClearSiteModes();
}

- (BOOL)externalLinkOnboardingShown {
  return _externalLinkHandler->IsOnboardingShown();
}

- (void)setExternalLinkOnboardingShown:(BOOL)shown {
  _externalLinkHandler->SetOnboardingShown(shown);
}

- (ExternalLinkAction)resolveExternalLinkForURL:(NSURL*)url {
  return static_cast<ExternalLinkAction>(
      _externalLinkHandler->Resolve(net::GURLWithNSURL(url)));
}

@end
