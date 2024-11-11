/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"

#include <algorithm>
#include <optional>

#include "base/containers/contains.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "url/gurl.h"

namespace {

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
  static const base::NoDestructor<std::vector<ContentSettingsType>>
      kShieldsContentSettingsTypes({
          ContentSettingsType::BRAVE_ADS,
          ContentSettingsType::BRAVE_COSMETIC_FILTERING,
          ContentSettingsType::BRAVE_TRACKERS,
          ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES,
          ContentSettingsType::BRAVE_FINGERPRINTING_V2,
          ContentSettingsType::BRAVE_SHIELDS,
          ContentSettingsType::BRAVE_REFERRERS,
          ContentSettingsType::BRAVE_COOKIES,
      });

  return *kShieldsContentSettingsTypes;
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
      break;
  }
  NOTREACHED() << "All handled shields content type above.";
}

bool IsShieldsContentSettingsType(const ContentSettingsType& content_type) {
  return base::Contains(GetShieldsContentSettingsTypes(), content_type);
}

bool IsShieldsContentSettingsTypeName(const std::string& content_type_name) {
  for (auto content_type : GetShieldsContentSettingsTypes()) {
    if (GetShieldsContentTypeName(content_type) == content_type_name)
      return true;
  }
  return false;
}

std::optional<ContentSettingsPattern> ConvertPatternToWildcardSchemeAndPort(
    const ContentSettingsPattern& pattern) {
  if (!CanPatternBeConvertedToWildcardSchemeAndPort(pattern))
    return std::nullopt;
  DCHECK(!pattern.GetHost().empty());
  std::optional<ContentSettingsPattern> new_pattern =
      ContentSettingsPattern::FromString("*://" + pattern.GetHost() + "/*");
  return new_pattern;
}

// Returns the full path in the user preferences store to the Brave Shields
// setting identified by it's name (i.e. |name|).
std::string GetShieldsSettingUserPrefsPath(const std::string& name) {
  return std::string("profile.content_settings.exceptions.").append(name);
}

// Extract a SessionModel from |dict[key]|. Will return
// SessionModel::Durable if no model exists.
content_settings::mojom::SessionModel GetSessionModelFromDictionary(
    const base::Value::Dict& dict,
    const char* key) {
  std::optional<int> model_int = dict.FindInt(key);
  if (!model_int.has_value() ||
      (model_int >
       static_cast<int>(content_settings::mojom::SessionModel::kMaxValue)) ||
      (model_int < 0)) {
    model_int = 0;
  }

  content_settings::mojom::SessionModel session_model =
      static_cast<content_settings::mojom::SessionModel>(model_int.value());
  return session_model;
}

}  // namespace content_settings
