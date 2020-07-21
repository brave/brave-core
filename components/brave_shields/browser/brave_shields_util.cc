/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_shields_util.h"

#include <memory>

#include "base/feature_list.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_perf_predictor/browser/buildflags.h"
#include "brave/components/brave_shields/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/common/brave_shield_utils.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/content_settings/core/common/content_settings_util.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/pref_names.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/referrer.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_BRAVE_PERF_PREDICTOR)
#include "brave/components/brave_perf_predictor/browser/perf_predictor_tab_helper.h"
#endif

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
      ContentSettingsType::PLUGINS, kBraveShields,
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
      ContentSettingsType::PLUGINS, kBraveShields,
      CONTENT_SETTING_DEFAULT);
}

bool GetBraveShieldsEnabled(HostContentSettingsMap* map, const GURL& url) {
  if (url.is_valid() && !url.SchemeIsHTTPOrHTTPS())
    return false;

  ContentSetting setting = map->GetContentSetting(
      url, GURL(), ContentSettingsType::PLUGINS, kBraveShields);

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

  map->SetContentSettingCustomScope(primary_pattern,
                                    ContentSettingsPattern::Wildcard(),
                                    ContentSettingsType::PLUGINS, kAds,
                                    GetDefaultBlockFromControlType(type));

  map->SetContentSettingCustomScope(primary_pattern,
                                    ContentSettingsPattern::Wildcard(),
                                    ContentSettingsType::PLUGINS, kTrackers,
                                    GetDefaultBlockFromControlType(type));
  RecordShieldsSettingChanged(local_state);
}

ControlType GetAdControlType(HostContentSettingsMap* map, const GURL& url) {
  ContentSetting setting = map->GetContentSetting(
      url, GURL(), ContentSettingsType::PLUGINS, kAds);

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

  map->SetContentSettingCustomScope(primary_pattern,
                                    ContentSettingsPattern::Wildcard(),
                                    ContentSettingsType::PLUGINS,
                                    kCosmeticFiltering,
                                    GetDefaultBlockFromControlType(type));

  map->SetContentSettingCustomScope(
      primary_pattern,
      ContentSettingsPattern::FromString("https://firstParty/*"),
      ContentSettingsType::PLUGINS,
      kCosmeticFiltering,
      GetDefaultAllowFromControlType(type));

  RecordShieldsSettingChanged(local_state);
}

ControlType GetCosmeticFilteringControlType(HostContentSettingsMap* map,
                                            const GURL& url) {
  ContentSetting setting = map->GetContentSetting(
      url, GURL(), ContentSettingsType::PLUGINS, kCosmeticFiltering);

  ContentSetting fp_setting = map->GetContentSetting(
      url,
      GURL("https://firstParty/"),
      ContentSettingsType::PLUGINS,
      kCosmeticFiltering);

  if (setting == CONTENT_SETTING_ALLOW) {
    return ControlType::ALLOW;
  } else if (fp_setting != CONTENT_SETTING_BLOCK) {
    return ControlType::BLOCK_THIRD_PARTY;
  } else {
    return ControlType::BLOCK;
  }
}

bool ShouldDoCosmeticFiltering(HostContentSettingsMap* map, const GURL& url) {
  return base::FeatureList::IsEnabled(features::kBraveAdblockCosmeticFiltering)
      && GetBraveShieldsEnabled(map, url)
      && (GetCosmeticFilteringControlType(map, url) != ControlType::ALLOW);
}

bool IsFirstPartyCosmeticFilteringEnabled(HostContentSettingsMap* map,
                                          const GURL& url) {
  const ControlType type = GetCosmeticFilteringControlType(map, url);
  return type == ControlType::BLOCK;
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
                                    ContentSettingsType::PLUGINS, kReferrers,
                                    GetDefaultBlockFromControlType(type));

  map->SetContentSettingCustomScope(
      primary_pattern,
      ContentSettingsPattern::FromString("https://firstParty/*"),
      ContentSettingsType::PLUGINS, kCookies,
      GetDefaultAllowFromControlType(type));

  map->SetContentSettingCustomScope(primary_pattern,
                                    ContentSettingsPattern::Wildcard(),
                                    ContentSettingsType::PLUGINS, kCookies,
                                    GetDefaultBlockFromControlType(type));

  RecordShieldsSettingChanged(local_state);
}
// TODO(bridiver) - convert cookie settings to ContentSettingsType::COOKIES
// while maintaining read backwards compat
ControlType GetCookieControlType(HostContentSettingsMap* map, const GURL& url) {
  ContentSetting setting = map->GetContentSetting(
      url, GURL(), ContentSettingsType::PLUGINS, kCookies);

  ContentSetting fp_setting = map->GetContentSetting(
      url,
      GURL("https://firstParty/"),
      ContentSettingsType::PLUGINS,
      kCookies);

  if (setting == CONTENT_SETTING_ALLOW) {
    return ControlType::ALLOW;
  } else if (fp_setting != CONTENT_SETTING_BLOCK) {
    return ControlType::BLOCK_THIRD_PARTY;
  } else {
    return ControlType::BLOCK;
  }
}

bool AllowReferrers(HostContentSettingsMap* map, const GURL& url) {
  ContentSetting setting = map->GetContentSetting(
      url, GURL(), ContentSettingsType::PLUGINS, kReferrers);

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
      primary_pattern,
      ContentSettingsPattern::FromString("https://balanced/*"),
      ContentSettingsType::PLUGINS,
      kFingerprintingV2,
      CONTENT_SETTING_DEFAULT);
  map->SetContentSettingCustomScope(primary_pattern,
                                    ContentSettingsPattern::Wildcard(),
                                    ContentSettingsType::PLUGINS,
                                    kFingerprintingV2,
                                    CONTENT_SETTING_DEFAULT);

  auto content_setting = CONTENT_SETTING_BLOCK;
  auto secondary_pattern =
      ContentSettingsPattern::FromString("https://balanced/*");

  if (type != ControlType::DEFAULT) {
    content_setting = GetDefaultBlockFromControlType(type);
    secondary_pattern = ContentSettingsPattern::Wildcard();
  }

  map->SetContentSettingCustomScope(primary_pattern,
                                    secondary_pattern,
                                    ContentSettingsType::PLUGINS,
                                    kFingerprintingV2,
                                    content_setting);

  RecordShieldsSettingChanged(local_state);
}

ControlType GetFingerprintingControlType(HostContentSettingsMap* map,
                                         const GURL& url) {
  ContentSettingsForOneType fingerprinting_rules;
  map->GetSettingsForOneType(ContentSettingsType::PLUGINS,
                             brave_shields::kFingerprintingV2,
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
      ContentSettingsType::PLUGINS, kHTTPUpgradableResources,
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
      ContentSettingsType::PLUGINS, kHTTPUpgradableResources,
      CONTENT_SETTING_DEFAULT);
}

bool GetHTTPSEverywhereEnabled(HostContentSettingsMap* map, const GURL& url) {
  ContentSetting setting = map->GetContentSetting(
      url, GURL(), ContentSettingsType::PLUGINS, kHTTPUpgradableResources);

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
      ContentSettingsType::JAVASCRIPT, "",
      type == ControlType::ALLOW ? CONTENT_SETTING_ALLOW
                                 : CONTENT_SETTING_BLOCK);
  RecordShieldsSettingChanged(local_state);
}

ControlType GetNoScriptControlType(HostContentSettingsMap* map,
                                   const GURL& url) {
  ContentSetting setting = map->GetContentSetting(
      url, GURL(), ContentSettingsType::JAVASCRIPT, "");

  return setting == CONTENT_SETTING_ALLOW ? ControlType::ALLOW
                                          : ControlType::BLOCK;
}

void DispatchBlockedEvent(const GURL& request_url,
                          int render_frame_id,
                          int render_process_id,
                          int frame_tree_node_id,
                          const std::string& block_type) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  BraveShieldsWebContentsObserver::DispatchBlockedEvent(
      block_type, request_url.spec(),
      render_process_id, render_frame_id, frame_tree_node_id);

#if BUILDFLAG(ENABLE_BRAVE_PERF_PREDICTOR)
  brave_perf_predictor::PerfPredictorTabHelper::DispatchBlockedEvent(
      request_url.spec(), render_process_id,
      render_frame_id, frame_tree_node_id);
#endif
}

bool MaybeChangeReferrer(
    bool allow_referrers,
    bool shields_up,
    const GURL& current_referrer,
    const GURL& tab_origin,
    const GURL& target_url,
    network::mojom::ReferrerPolicy policy,
    Referrer* output_referrer) {
  DCHECK(output_referrer);
  if (allow_referrers || !shields_up || current_referrer.is_empty()) {
    return false;
  }

  const url::Origin original_referrer = url::Origin::Create(current_referrer);
  const url::Origin target_origin = url::Origin::Create(target_url);

  if (original_referrer.IsSameOriginWith(target_origin)) {
    // Do nothing for same-origin requests. This check also prevents us from
    // sending referrer from HTTPS to HTTP.
    return false;
  }

  // Cap referrer to "strict-origin-when-cross-origin" or anything more
  // restrictive according do given policy.
  *output_referrer = Referrer::SanitizeForRequest(
      target_url, Referrer(current_referrer.GetOrigin(), policy));

  return true;
}

}  // namespace brave_shields
