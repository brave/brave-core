/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/external_links/core/browser/external_link_handler.h"

#include "base/check.h"
#include "base/values.h"
#include "brave/components/content_settings/core/common/content_settings_util.h"
#include "brave/components/external_links/core/common/external_link_mode_value.h"
#include "brave/components/external_links/core/common/pref_names.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace external_links {

namespace {

constexpr ContentSettingsType kType =
    ContentSettingsType::BRAVE_EXTERNAL_LINK_MODE;

bool IsDefaultPattern(const ContentSettingsPattern& primary,
                      const ContentSettingsPattern& secondary) {
  return primary == ContentSettingsPattern::Wildcard() &&
         secondary == ContentSettingsPattern::Wildcard();
}

}  // namespace

ExternalLinkHandler::ExternalLinkHandler(HostContentSettingsMap* settings_map,
                                         PrefService* prefs)
    : settings_map_(settings_map), prefs_(prefs) {
  CHECK(settings_map_);
  CHECK(prefs_);
}

ExternalLinkHandler::~ExternalLinkHandler() = default;

// static
void ExternalLinkHandler::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kOnboardingShown, false);
}

ExternalLinkMode ExternalLinkHandler::GetDefaultMode() const {
  return ExternalLinkModeFromValue(
             settings_map_->GetWebsiteSetting(GURL(), GURL(), kType))
      .value_or(ExternalLinkMode::kCurrent);
}

void ExternalLinkHandler::SetDefaultMode(ExternalLinkMode mode) {
  settings_map_->SetWebsiteSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      kType, ExternalLinkModeToValue(mode));
}

ExternalLinkMode ExternalLinkHandler::GetMode(const GURL& url) const {
  if (!url.is_valid()) {
    return GetDefaultMode();
  }
  return ExternalLinkModeFromValue(
             settings_map_->GetWebsiteSetting(url, url, kType))
      .value_or(ExternalLinkMode::kCurrent);
}

void ExternalLinkHandler::SetSiteMode(const GURL& url, ExternalLinkMode mode) {
  if (!url.is_valid()) {
    return;
  }
  settings_map_->SetWebsiteSettingCustomScope(
      content_settings::CreateDomainPattern(url),
      ContentSettingsPattern::Wildcard(), kType, ExternalLinkModeToValue(mode));
}

void ExternalLinkHandler::ClearSiteModes() {
  for (const auto& entry : settings_map_->GetSettingsForOneType(kType)) {
    if (IsDefaultPattern(entry.primary_pattern, entry.secondary_pattern)) {
      continue;
    }
    settings_map_->SetWebsiteSettingCustomScope(
        entry.primary_pattern, entry.secondary_pattern, kType, base::Value());
  }
}

bool ExternalLinkHandler::IsOnboardingShown() const {
  return prefs_->GetBoolean(prefs::kOnboardingShown);
}

void ExternalLinkHandler::SetOnboardingShown(bool shown) {
  prefs_->SetBoolean(prefs::kOnboardingShown, shown);
}

ExternalLinkAction ExternalLinkHandler::Resolve(const GURL& url) {
  if (!url.SchemeIsHTTPOrHTTPS()) {
    return ExternalLinkAction::kOpenCurrent;
  }
  switch (GetMode(url)) {
    case ExternalLinkMode::kCurrent:
      if (IsOnboardingShown()) {
        return ExternalLinkAction::kOpenCurrent;
      }
      SetOnboardingShown(true);
      return ExternalLinkAction::kShowOnboarding;
    case ExternalLinkMode::kAsk:
      return ExternalLinkAction::kShowPrompt;
    case ExternalLinkMode::kRegular:
      return ExternalLinkAction::kOpenRegular;
    case ExternalLinkMode::kPrivateBrowsing:
      return ExternalLinkAction::kOpenPrivate;
    case ExternalLinkMode::kModal:
      return ExternalLinkAction::kOpenModal;
  }
}

}  // namespace external_links
