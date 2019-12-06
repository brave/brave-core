/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_shields_util.h"

#include <memory>

#include "base/strings/string_number_conversions.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/shield_exceptions.h"
#include "brave/components/brave_shields/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/browser/referrer_whitelist_service.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/content_settings/core/common/content_settings_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/pref_names.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/referrer.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

using content::Referrer;

namespace brave_shields {

namespace {

void RecordShieldsToggled() {
  PrefService* local_state = g_browser_process->local_state();
  ::brave_shields::MaybeRecordShieldsUsageP3A(::brave_shields::kShutOffShields,
                                              local_state);
}

void RecordShieldsSettingChanged() {
  PrefService* local_state = g_browser_process->local_state();
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

ContentSettingsPattern GetPatternFromURL(const GURL& url,
                                         bool scheme_wildcard) {
  DCHECK(url.is_empty() ? url.possibly_invalid_spec() == "" : url.is_valid());
  if (url.is_empty() && url.possibly_invalid_spec() == "")
    return ContentSettingsPattern::Wildcard();
  auto origin = url.GetOrigin();
  return scheme_wildcard && !url.has_port()
             ? ContentSettingsPattern::FromString("*://" + url.host() + "/*")
             : ContentSettingsPattern::FromString(
                   origin.scheme() + "://" + origin.host() + ":" +
                   base::NumberToString(origin.EffectiveIntPort()) + "/*");
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

void SetBraveShieldsEnabled(Profile* profile,
                        bool enable,
                        const GURL& url) {
  if (url.is_valid() && !url.SchemeIsHTTPOrHTTPS())
    return;

  DCHECK(!url.is_empty()) << "url for shields setting cannot be blank";

  auto primary_pattern = GetPatternFromURL(url, true);

  if (!primary_pattern.IsValid())
    return;

  HostContentSettingsMapFactory::GetForProfile(profile)
      ->SetContentSettingCustomScope(
          primary_pattern, ContentSettingsPattern::Wildcard(),
          ContentSettingsType::PLUGINS, kBraveShields,
          // this is 'allow_brave_shields' so 'enable' == 'allow'
          enable ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK);

  RecordShieldsToggled();
}

void ResetBraveShieldsEnabled(Profile* profile,
                              const GURL& url) {
  if (url.is_valid() && !url.SchemeIsHTTPOrHTTPS())
    return;

  auto primary_pattern = GetPatternFromURL(url, true);

  if (!primary_pattern.IsValid())
    return;

  HostContentSettingsMapFactory::GetForProfile(profile)
      ->SetContentSettingCustomScope(
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

bool GetBraveShieldsEnabled(Profile* profile, const GURL& url) {
  return GetBraveShieldsEnabled(
      HostContentSettingsMapFactory::GetForProfile(profile), url);
}

void SetAdControlType(Profile* profile, ControlType type, const GURL& url) {
  DCHECK(type != ControlType::BLOCK_THIRD_PARTY);
  auto primary_pattern = GetPatternFromURL(url);

  if (!primary_pattern.IsValid()) {
    return;
  }

  HostContentSettingsMapFactory::GetForProfile(profile)
      ->SetContentSettingCustomScope(primary_pattern,
                                     ContentSettingsPattern::Wildcard(),
                                     ContentSettingsType::PLUGINS, kAds,
                                     GetDefaultBlockFromControlType(type));

  HostContentSettingsMapFactory::GetForProfile(profile)
      ->SetContentSettingCustomScope(primary_pattern,
                                     ContentSettingsPattern::Wildcard(),
                                     ContentSettingsType::PLUGINS, kTrackers,
                                     GetDefaultBlockFromControlType(type));
  RecordShieldsSettingChanged();
}

ControlType GetAdControlType(Profile* profile, const GURL& url) {
  ContentSetting setting =
      HostContentSettingsMapFactory::GetForProfile(profile)->GetContentSetting(
          url, GURL(), ContentSettingsType::PLUGINS, kAds);

  return setting == CONTENT_SETTING_ALLOW ? ControlType::ALLOW
                                          : ControlType::BLOCK;
}

// TODO(bridiver) - convert cookie settings to ContentSettingsType::COOKIES
// while maintaining read backwards compat
void SetCookieControlType(Profile* profile, ControlType type, const GURL& url) {
  return SetCookieControlType(
      HostContentSettingsMapFactory::GetForProfile(profile),
      type,
      url);
}

ControlType GetCookieControlType(Profile* profile, const GURL& url) {
  return GetCookieControlType(
      HostContentSettingsMapFactory::GetForProfile(profile), url);
}

void SetCookieControlType(HostContentSettingsMap* map,
                          ControlType type,
                          const GURL& url) {
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

  RecordShieldsSettingChanged();
}

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

bool AllowReferrers(Profile* profile, const GURL& url) {
  return AllowReferrers(
      HostContentSettingsMapFactory::GetForProfile(profile), url);
}

bool AllowReferrers(HostContentSettingsMap* map, const GURL& url) {
  ContentSetting setting = map->GetContentSetting(
      url, GURL(), ContentSettingsType::PLUGINS, kReferrers);

  return setting == CONTENT_SETTING_ALLOW;
}

void SetFingerprintingControlType(Profile* profile,
                                  ControlType type,
                                  const GURL& url) {
  auto primary_pattern = GetPatternFromURL(url);

  if (!primary_pattern.IsValid())
    return;

  auto* map = HostContentSettingsMapFactory::GetForProfile(profile);
  map->SetContentSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::PLUGINS, kFingerprinting,
      GetDefaultBlockFromControlType(type));

  map->SetContentSettingCustomScope(
      primary_pattern,
      ContentSettingsPattern::FromString("https://firstParty/*"),
      ContentSettingsType::PLUGINS, kFingerprinting,
      GetDefaultAllowFromControlType(type));

  RecordShieldsSettingChanged();
}

ControlType GetFingerprintingControlType(Profile* profile, const GURL& url) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile);

  ContentSetting setting = map->GetContentSetting(
      url, GURL(), ContentSettingsType::PLUGINS, kFingerprinting);
  ContentSetting fp_setting =
      map->GetContentSetting(url, GURL("https://firstParty/"),
                             ContentSettingsType::PLUGINS, kFingerprinting);

  if (setting != fp_setting || setting == CONTENT_SETTING_DEFAULT) {
    return ControlType::BLOCK_THIRD_PARTY;
  } else {
    return setting == CONTENT_SETTING_ALLOW ? ControlType::ALLOW
                                            : ControlType::BLOCK;
  }
}

void SetHTTPSEverywhereEnabled(Profile* profile,
                               bool enable,
                               const GURL& url) {
  auto primary_pattern = GetPatternFromURL(url, true);

  if (!primary_pattern.IsValid())
    return;

  HostContentSettingsMapFactory::GetForProfile(profile)
      ->SetContentSettingCustomScope(
          primary_pattern, ContentSettingsPattern::Wildcard(),
          ContentSettingsType::PLUGINS, kHTTPUpgradableResources,
          // this is 'allow_http_upgradeable_resources' so enabling
          // httpse will set the value to 'BLOCK'
          enable ? CONTENT_SETTING_BLOCK : CONTENT_SETTING_ALLOW);
  RecordShieldsSettingChanged();
}

void ResetHTTPSEverywhereEnabled(Profile* profile,
                               bool enable,
                               const GURL& url) {
  auto primary_pattern = GetPatternFromURL(url, true);

  if (!primary_pattern.IsValid())
    return;

  HostContentSettingsMapFactory::GetForProfile(profile)
      ->SetContentSettingCustomScope(
          primary_pattern, ContentSettingsPattern::Wildcard(),
          ContentSettingsType::PLUGINS, kHTTPUpgradableResources,
          CONTENT_SETTING_DEFAULT);
}

bool GetHTTPSEverywhereEnabled(Profile* profile, const GURL& url) {
  ContentSetting setting =
      HostContentSettingsMapFactory::GetForProfile(profile)->GetContentSetting(
          url, GURL(), ContentSettingsType::PLUGINS, kHTTPUpgradableResources);

  return setting == CONTENT_SETTING_ALLOW ? false : true;
}

void SetNoScriptControlType(Profile* profile,
                            ControlType type,
                            const GURL& url) {
  DCHECK(type != ControlType::BLOCK_THIRD_PARTY);
  auto primary_pattern = GetPatternFromURL(url);

  if (!primary_pattern.IsValid())
    return;

  HostContentSettingsMapFactory::GetForProfile(profile)
      ->SetContentSettingCustomScope(
          primary_pattern, ContentSettingsPattern::Wildcard(),
          ContentSettingsType::JAVASCRIPT, "",
          type == ControlType::ALLOW ? CONTENT_SETTING_ALLOW
                                     : CONTENT_SETTING_BLOCK);
  RecordShieldsSettingChanged();
}

ControlType GetNoScriptControlType(Profile* profile, const GURL& url) {
  ContentSetting setting =
      HostContentSettingsMapFactory::GetForProfile(profile)->GetContentSetting(
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
}

bool ShouldSetReferrer(bool allow_referrers,
                       bool shields_up,
                       const GURL& original_referrer,
                       const GURL& tab_origin,
                       const GURL& target_url,
                       const GURL& new_referrer_url,
                       network::mojom::ReferrerPolicy policy,
                       Referrer* output_referrer) {
  if (!output_referrer || allow_referrers || !shields_up ||
      original_referrer.is_empty() ||
      // Same TLD+1 whouldn't set the referrer
      SameDomainOrHost(
          target_url, original_referrer,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES) ||
      // Whitelisted referrers shoud never set the referrer
      (g_brave_browser_process &&
       g_brave_browser_process->referrer_whitelist_service()->IsWhitelisted(
           tab_origin, target_url.GetOrigin()))) {
    return false;
  }
  *output_referrer = Referrer::SanitizeForRequest(
      target_url, Referrer(new_referrer_url, policy));
  return true;
}

}  // namespace brave_shields
