/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_shields_util.h"

#include <memory>

#include "base/feature_list.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_shields/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/common/brave_shield_utils.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/content_settings/core/common/content_settings_util.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/core/common/pref_names.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/referrer.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

using content::Referrer;

namespace brave_shields {

namespace {

void RecordShieldsToggled(PrefService* local_state) {
  ::brave_shields::MaybeRecordShieldsUsageP3A(::brave_shields::kShutOffShields,
                                              local_state);
}

void RecordShieldsSettingChanged(PrefService* local_state) {
  ::brave_shields::MaybeRecordShieldsUsageP3A(
      ::brave_shields::kChangedPerSiteShields, local_state);
}

ContentSetting GetDefaultAllowFromControlType(ControlType type) {
  if (type == ControlType::DEFAULT)
    return CONTENT_SETTING_DEFAULT;

  return type == ControlType::BLOCK ? CONTENT_SETTING_BLOCK
                                    : CONTENT_SETTING_ALLOW;
}

ContentSetting GetDefaultBlockFromControlType(ControlType type) {
  if (type == ControlType::DEFAULT)
    return CONTENT_SETTING_DEFAULT;

  return type == ControlType::ALLOW ? CONTENT_SETTING_ALLOW
                                    : CONTENT_SETTING_BLOCK;
}

}  // namespace

ContentSettingsPattern GetPatternFromURL(const GURL& url) {
  DCHECK(url.is_empty() ? url.possibly_invalid_spec() == "" : url.is_valid());
  if (url.is_empty() && url.possibly_invalid_spec() == "")
    return ContentSettingsPattern::Wildcard();
  return ContentSettingsPattern::FromString("*://" + url.host() + "/*");
}

std::string ControlTypeToString(ControlType type) {
  switch (type) {
    case ControlType::ALLOW:
      return "allow";
    case ControlType::BLOCK:
      return "block";
    case ControlType::AGGRESSIVE:
      return "aggressive";
    case ControlType::BLOCK_THIRD_PARTY:
      return "block_third_party";
    case ControlType::DEFAULT:
      return "default";
    default:
      NOTREACHED();
      return "invalid";
  }
}

ControlType ControlTypeFromString(const std::string& string) {
  if (string == "allow") {
    return ControlType::ALLOW;
  } else if (string == "block") {
    return ControlType::BLOCK;
  } else if (string == "aggressive") {
    return ControlType::AGGRESSIVE;
  } else if (string == "block_third_party") {
    return ControlType::BLOCK_THIRD_PARTY;
  } else if (string == "default") {
    return ControlType::DEFAULT;
  } else {
    NOTREACHED();
    return ControlType::INVALID;
  }
}

void SetBraveShieldsEnabled(HostContentSettingsMap* map,
                            bool enable,
                            const GURL& url,
                            PrefService* local_state) {
  if (url.is_valid() && !url.SchemeIsHTTPOrHTTPS())
    return;

  DCHECK(!url.is_empty()) << "url for shields setting cannot be blank";

  auto primary_pattern = GetPatternFromURL(url);

  if (!primary_pattern.IsValid())
    return;

  map->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_SHIELDS,
      // this is 'allow_brave_shields' so 'enable' == 'allow'
      enable ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK);

  RecordShieldsToggled(local_state);
}

void ResetBraveShieldsEnabled(HostContentSettingsMap* map, const GURL& url) {
  if (url.is_valid() && !url.SchemeIsHTTPOrHTTPS())
    return;

  auto primary_pattern = GetPatternFromURL(url);

  if (!primary_pattern.IsValid())
    return;

  map->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_SHIELDS, CONTENT_SETTING_DEFAULT);
}

bool GetBraveShieldsEnabled(HostContentSettingsMap* map, const GURL& url) {
  if (url.is_valid() && !url.SchemeIsHTTPOrHTTPS())
    return false;

  ContentSetting setting =
      map->GetContentSetting(url, GURL(), ContentSettingsType::BRAVE_SHIELDS);

  // see EnableBraveShields - allow and default == true
  return setting == CONTENT_SETTING_BLOCK ? false : true;
}

void SetAdControlType(HostContentSettingsMap* map,
                      ControlType type,
                      const GURL& url,
                      PrefService* local_state) {
  DCHECK(type != ControlType::BLOCK_THIRD_PARTY);
  auto primary_pattern = GetPatternFromURL(url);

  if (!primary_pattern.IsValid()) {
    return;
  }

  map->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_ADS, GetDefaultBlockFromControlType(type));

  map->SetContentSettingCustomScope(primary_pattern,
                                    ContentSettingsPattern::Wildcard(),
                                    ContentSettingsType::BRAVE_TRACKERS,
                                    GetDefaultBlockFromControlType(type));
  RecordShieldsSettingChanged(local_state);
}

ControlType GetAdControlType(HostContentSettingsMap* map, const GURL& url) {
  ContentSetting setting =
      map->GetContentSetting(url, GURL(), ContentSettingsType::BRAVE_ADS);

  return setting == CONTENT_SETTING_ALLOW ? ControlType::ALLOW
                                          : ControlType::BLOCK;
}

void SetCosmeticFilteringControlType(HostContentSettingsMap* map,
                                     ControlType type,
                                     const GURL& url,
                                     PrefService* local_state) {
  auto primary_pattern = GetPatternFromURL(url);

  if (!primary_pattern.IsValid()) {
    return;
  }

  map->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_COSMETIC_FILTERING,
      GetDefaultBlockFromControlType(type));

  map->SetContentSettingCustomScope(
      primary_pattern,
      ContentSettingsPattern::FromString("https://firstParty/*"),
      ContentSettingsType::BRAVE_COSMETIC_FILTERING,
      GetDefaultAllowFromControlType(type));

  RecordShieldsSettingChanged(local_state);
}

ControlType GetCosmeticFilteringControlType(HostContentSettingsMap* map,
                                            const GURL& url) {
  ContentSetting setting = map->GetContentSetting(
      url, GURL(), ContentSettingsType::BRAVE_COSMETIC_FILTERING);

  ContentSetting fp_setting =
      map->GetContentSetting(url, GURL("https://firstParty/"),
                             ContentSettingsType::BRAVE_COSMETIC_FILTERING);

  if (setting == CONTENT_SETTING_ALLOW) {
    return ControlType::ALLOW;
  } else if (fp_setting != CONTENT_SETTING_BLOCK) {
    return ControlType::BLOCK_THIRD_PARTY;
  } else {
    return ControlType::BLOCK;
  }
}

bool ShouldDoCosmeticFiltering(HostContentSettingsMap* map, const GURL& url) {
  return base::FeatureList::IsEnabled(
             features::kBraveAdblockCosmeticFiltering) &&
         GetBraveShieldsEnabled(map, url) &&
         (GetCosmeticFilteringControlType(map, url) != ControlType::ALLOW);
}

bool IsFirstPartyCosmeticFilteringEnabled(HostContentSettingsMap* map,
                                          const GURL& url) {
  const ControlType type = GetCosmeticFilteringControlType(map, url);
  return type == ControlType::BLOCK;
}

bool ShouldDoDomainBlocking(HostContentSettingsMap* map, const GURL& url) {
  // Don't block if feature is disabled
  if (!base::FeatureList::IsEnabled(brave_shields::features::kBraveDomainBlock))
    return false;

  // Don't block if Brave Shields is down (this also handles cases where
  // the URL is not HTTP(S))
  if (!brave_shields::GetBraveShieldsEnabled(map, url))
    return false;

  // Don't block unless ad blocking is "aggressive"
  if (brave_shields::GetCosmeticFilteringControlType(map, url) !=
      ControlType::BLOCK)
    return false;

  return true;
}

void SetCookieControlType(HostContentSettingsMap* map,
                          ControlType type,
                          const GURL& url,
                          PrefService* local_state) {
  auto primary_pattern = GetPatternFromURL(url);

  if (!primary_pattern.IsValid())
    return;

  map->SetContentSettingCustomScope(primary_pattern,
                                    ContentSettingsPattern::Wildcard(),
                                    ContentSettingsType::BRAVE_REFERRERS,
                                    GetDefaultBlockFromControlType(type));

  map->SetContentSettingCustomScope(
      primary_pattern,
      ContentSettingsPattern::FromString("https://firstParty/*"),
      ContentSettingsType::BRAVE_COOKIES, GetDefaultAllowFromControlType(type));

  map->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_COOKIES, GetDefaultBlockFromControlType(type));

  RecordShieldsSettingChanged(local_state);
}
// TODO(bridiver) - convert cookie settings to ContentSettingsType::COOKIES
// while maintaining read backwards compat
ControlType GetCookieControlType(HostContentSettingsMap* map, const GURL& url) {
  ContentSetting setting =
      map->GetContentSetting(url, GURL(), ContentSettingsType::BRAVE_COOKIES);

  ContentSetting fp_setting = map->GetContentSetting(
      url, GURL("https://firstParty/"), ContentSettingsType::BRAVE_COOKIES);

  if (setting == CONTENT_SETTING_ALLOW) {
    return ControlType::ALLOW;
  } else if (fp_setting != CONTENT_SETTING_BLOCK) {
    return ControlType::BLOCK_THIRD_PARTY;
  } else {
    return ControlType::BLOCK;
  }
}

bool AllowReferrers(HostContentSettingsMap* map, const GURL& url) {
  ContentSetting setting =
      map->GetContentSetting(url, GURL(), ContentSettingsType::BRAVE_REFERRERS);

  return setting == CONTENT_SETTING_ALLOW;
}

void SetFingerprintingControlType(HostContentSettingsMap* map,
                                  ControlType type,
                                  const GURL& url,
                                  PrefService* local_state) {
  auto primary_pattern = GetPatternFromURL(url);

  if (!primary_pattern.IsValid())
    return;

  // Clear previous value to have only one rule for one pattern.
  map->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::FromString("https://balanced/*"),
      ContentSettingsType::BRAVE_FINGERPRINTING_V2, CONTENT_SETTING_DEFAULT);
  map->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_FINGERPRINTING_V2, CONTENT_SETTING_DEFAULT);

  auto content_setting = CONTENT_SETTING_BLOCK;
  auto secondary_pattern =
      ContentSettingsPattern::FromString("https://balanced/*");

  if (type != ControlType::DEFAULT) {
    content_setting = GetDefaultBlockFromControlType(type);
    secondary_pattern = ContentSettingsPattern::Wildcard();
  }

  map->SetContentSettingCustomScope(
      primary_pattern, secondary_pattern,
      ContentSettingsType::BRAVE_FINGERPRINTING_V2, content_setting);

  RecordShieldsSettingChanged(local_state);
}

ControlType GetFingerprintingControlType(HostContentSettingsMap* map,
                                         const GURL& url) {
  ContentSettingsForOneType fingerprinting_rules;
  map->GetSettingsForOneType(ContentSettingsType::BRAVE_FINGERPRINTING_V2,
                             &fingerprinting_rules);

  ContentSetting fp_setting =
      GetBraveFPContentSettingFromRules(fingerprinting_rules, url);
  if (fp_setting == CONTENT_SETTING_DEFAULT)
    return ControlType::DEFAULT;
  return fp_setting == CONTENT_SETTING_ALLOW ? ControlType::ALLOW
                                             : ControlType::BLOCK;
}

void SetHTTPSEverywhereEnabled(HostContentSettingsMap* map,
                               bool enable,
                               const GURL& url,
                               PrefService* local_state) {
  auto primary_pattern = GetPatternFromURL(url);

  if (!primary_pattern.IsValid())
    return;

  map->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES,
      // this is 'allow_http_upgradeable_resources' so enabling
      // httpse will set the value to 'BLOCK'
      enable ? CONTENT_SETTING_BLOCK : CONTENT_SETTING_ALLOW);

  RecordShieldsSettingChanged(local_state);
}

void ResetHTTPSEverywhereEnabled(HostContentSettingsMap* map,
                                 bool enable,
                                 const GURL& url) {
  auto primary_pattern = GetPatternFromURL(url);

  if (!primary_pattern.IsValid())
    return;

  map->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES,
      CONTENT_SETTING_DEFAULT);
}

bool GetHTTPSEverywhereEnabled(HostContentSettingsMap* map, const GURL& url) {
  ContentSetting setting = map->GetContentSetting(
      url, GURL(), ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES);

  return setting == CONTENT_SETTING_ALLOW ? false : true;
}

void SetNoScriptControlType(HostContentSettingsMap* map,
                            ControlType type,
                            const GURL& url,
                            PrefService* local_state) {
  DCHECK(type != ControlType::BLOCK_THIRD_PARTY);
  auto primary_pattern = GetPatternFromURL(url);

  if (!primary_pattern.IsValid())
    return;

  map->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::JAVASCRIPT,
      type == ControlType::ALLOW ? CONTENT_SETTING_ALLOW
                                 : CONTENT_SETTING_BLOCK);
  RecordShieldsSettingChanged(local_state);
}

ControlType GetNoScriptControlType(HostContentSettingsMap* map,
                                   const GURL& url) {
  ContentSetting setting =
      map->GetContentSetting(url, GURL(), ContentSettingsType::JAVASCRIPT);

  return setting == CONTENT_SETTING_ALLOW ? ControlType::ALLOW
                                          : ControlType::BLOCK;
}

bool IsSameOriginNavigation(const GURL& referrer, const GURL& target_url) {
  const url::Origin original_referrer = url::Origin::Create(referrer);
  const url::Origin target_origin = url::Origin::Create(target_url);

  return original_referrer.IsSameOriginWith(target_origin);
}

bool MaybeChangeReferrer(bool allow_referrers,
                         bool shields_up,
                         const GURL& current_referrer,
                         const GURL& target_url,
                         Referrer* output_referrer) {
  DCHECK(output_referrer);
  if (allow_referrers || !shields_up || current_referrer.is_empty()) {
    return false;
  }

  if (IsSameOriginNavigation(current_referrer, target_url)) {
    // Do nothing for same-origin requests. This check also prevents us from
    // sending referrer from HTTPS to HTTP.
    return false;
  }

  // Cap the referrer to "strict-origin-when-cross-origin". More restrictive
  // policies should be already applied.
  // See https://github.com/brave/brave-browser/issues/13464
  *output_referrer = Referrer::SanitizeForRequest(
      target_url,
      Referrer(current_referrer.GetOrigin(),
               network::mojom::ReferrerPolicy::kStrictOriginWhenCrossOrigin));

  return true;
}

}  // namespace brave_shields
