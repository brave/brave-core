/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"

#include <algorithm>

#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace {

const std::vector<ContentSettingsType> kShieldsContentSettingsTypes{
    ContentSettingsType::BRAVE_ADS,
    ContentSettingsType::BRAVE_COSMETIC_FILTERING,
    ContentSettingsType::BRAVE_TRACKERS,
    ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES,
    ContentSettingsType::BRAVE_FINGERPRINTING_V2,
    ContentSettingsType::BRAVE_SHIELDS,
    ContentSettingsType::BRAVE_REFERRERS,
    ContentSettingsType::BRAVE_COOKIES};

bool CanPatternBeConvertedToWildcardSchemeAndPort(
    const ContentSettingsPattern& pattern) {
  // 1. Wildcard is already in the desired state.
  // 2. Our firstParty placeholder shouldn't be converted.
  // 3. Patterns that have file:// scheme.
  // 4. We only want to convert patterns that have a specific host, so something
  // like "http://*:80/*" should be left alone.
  if (pattern == ContentSettingsPattern::Wildcard() ||
      pattern == ContentSettingsPattern::FromString("https://firstParty/*") ||
      pattern.GetScheme() == ContentSettingsPattern::SCHEME_FILE ||
      pattern.MatchesAllHosts() || pattern.GetHost().empty())
    return false;
  // Check for the case when the scheme is wildcard, but the port isn't.
  if (pattern.GetScheme() == ContentSettingsPattern::SCHEME_WILDCARD) {
    GURL check_for_port_url("http://" + pattern.ToString());
    return check_for_port_url.has_port();
  }
  GURL url(pattern.ToString());
  if (!url.is_valid() || url.is_empty() || !url.has_host())
    return false;
  if (url.has_scheme())
    return !ContentSettingsPattern::IsNonWildcardDomainNonPortScheme(
        url.scheme_piece());
  return url.has_port();
}

}  // namespace

namespace content_settings {

const std::vector<ContentSettingsType>& GetShieldsContentSettingsTypes() {
  return kShieldsContentSettingsTypes;
}

std::string GetShieldsContentTypeName(const ContentSettingsType& content_type) {
  switch (content_type) {
    case ContentSettingsType::BRAVE_ADS:
      return brave_shields::kAds;
    case ContentSettingsType::BRAVE_COSMETIC_FILTERING:
      return brave_shields::kCosmeticFiltering;
    case ContentSettingsType::BRAVE_TRACKERS:
      return brave_shields::kTrackers;
    case ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES:
      return brave_shields::kHTTPUpgradableResources;
    case ContentSettingsType::BRAVE_FINGERPRINTING_V2:
      return brave_shields::kFingerprintingV2;
    case ContentSettingsType::BRAVE_SHIELDS:
      return brave_shields::kBraveShields;
    case ContentSettingsType::BRAVE_REFERRERS:
      return brave_shields::kReferrers;
    case ContentSettingsType::BRAVE_COOKIES:
      return brave_shields::kCookies;
    default:
      NOTREACHED();
      return std::string();
  }

  NOTREACHED();
  return std::string();
}

bool IsShieldsContentSettingsType(const ContentSettingsType& content_type) {
  return std::find(kShieldsContentSettingsTypes.begin(),
                   kShieldsContentSettingsTypes.end(),
                   content_type) != kShieldsContentSettingsTypes.end();
}

bool IsShieldsContentSettingsTypeName(const std::string& content_type_name) {
  for (auto content_type : kShieldsContentSettingsTypes) {
    if (GetShieldsContentTypeName(content_type) == content_type_name)
      return true;
  }
  return false;
}

absl::optional<ContentSettingsPattern> ConvertPatternToWildcardSchemeAndPort(
    const ContentSettingsPattern& pattern) {
  if (!CanPatternBeConvertedToWildcardSchemeAndPort(pattern))
    return absl::nullopt;
  DCHECK(!pattern.GetHost().empty());
  absl::optional<ContentSettingsPattern> new_pattern =
      ContentSettingsPattern::FromString("*://" + pattern.GetHost() + "/*");
  return new_pattern;
}

// Returns the full path in the user preferences store to the Brave Shields
// setting identified by it's name (i.e. |name|).
std::string GetShieldsSettingUserPrefsPath(const std::string& name) {
  return std::string("profile.content_settings.exceptions.").append(name);
}

// Extract a timestamp from |dictionary[key]|. Will return base::Time() if no
// timestamp exists.
base::Time GetTimeStampFromDictionary(const base::DictionaryValue* dictionary,
                                      const char* key) {
  std::string timestamp_str;
  dictionary->GetStringWithoutPathExpansion(key, &timestamp_str);
  int64_t timestamp = 0;
  base::StringToInt64(timestamp_str, &timestamp);
  base::Time last_modified = base::Time::FromDeltaSinceWindowsEpoch(
      base::TimeDelta::FromMicroseconds(timestamp));
  return last_modified;
}

// Extract a SessionModel from |dictionary[key]|. Will return
// SessionModel::Durable if no model exists.
content_settings::SessionModel GetSessionModelFromDictionary(
    const base::DictionaryValue* dictionary,
    const char* key) {
  int model_int = 0;
  dictionary->GetIntegerWithoutPathExpansion(key, &model_int);
  if ((model_int >
       static_cast<int>(content_settings::SessionModel::kMaxValue)) ||
      (model_int < 0)) {
    model_int = 0;
  }

  content_settings::SessionModel session_model =
      static_cast<content_settings::SessionModel>(model_int);
  return session_model;
}

}  // namespace content_settings
