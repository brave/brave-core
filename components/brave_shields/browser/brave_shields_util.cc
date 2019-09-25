/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_shields_util.h"

#include <memory>

#include "base/strings/string_number_conversions.h"
#include "base/task/post_task.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/shield_exceptions.h"
#include "brave/components/brave_shields/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/browser/referrer_whitelist_service.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/content_settings/core/common/content_settings_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/websocket_handshake_request_info.h"
#include "content/public/common/referrer.h"
#include "extensions/browser/extension_api_frame_id_map.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

using content::BrowserThread;
using content::Referrer;
using content::ResourceContext;
//using content::ResourceRequestInfo;
using net::URLRequest;

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

  auto primary_pattern = GetPatternFromURL(url, true);

  if (!primary_pattern.IsValid())
    return;

  HostContentSettingsMapFactory::GetForProfile(profile)
      ->SetContentSettingCustomScope(
          primary_pattern, ContentSettingsPattern::Wildcard(),
          CONTENT_SETTINGS_TYPE_PLUGINS, kBraveShields,
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
          CONTENT_SETTINGS_TYPE_PLUGINS, kBraveShields,
          CONTENT_SETTING_DEFAULT);
}

bool GetBraveShieldsEnabled(HostContentSettingsMap* map, const GURL& url) {
  if (url.is_valid() && !url.SchemeIsHTTPOrHTTPS())
    return false;

  ContentSetting setting = map->GetContentSetting(
      url, GURL(), CONTENT_SETTINGS_TYPE_PLUGINS, kBraveShields);

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
                                     CONTENT_SETTINGS_TYPE_PLUGINS, kAds,
                                     GetDefaultBlockFromControlType(type));

  HostContentSettingsMapFactory::GetForProfile(profile)
      ->SetContentSettingCustomScope(primary_pattern,
                                     ContentSettingsPattern::Wildcard(),
                                     CONTENT_SETTINGS_TYPE_PLUGINS, kTrackers,
                                     GetDefaultBlockFromControlType(type));
  RecordShieldsSettingChanged();
}

ControlType GetAdControlType(Profile* profile, const GURL& url) {
  ContentSetting setting =
      HostContentSettingsMapFactory::GetForProfile(profile)->GetContentSetting(
          url, GURL(), CONTENT_SETTINGS_TYPE_PLUGINS, kAds);

  return setting == CONTENT_SETTING_ALLOW ? ControlType::ALLOW
                                          : ControlType::BLOCK;
}

// TODO(bridiver) - convert cookie settings to CONTENT_SETTINGS_TYPE_COOKIES
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
                                    CONTENT_SETTINGS_TYPE_PLUGINS, kReferrers,
                                    GetDefaultBlockFromControlType(type));

  map->SetContentSettingCustomScope(primary_pattern,
                                    ContentSettingsPattern::Wildcard(),
                                    CONTENT_SETTINGS_TYPE_PLUGINS, kCookies,
                                    GetDefaultBlockFromControlType(type));

  map->SetContentSettingCustomScope(
      primary_pattern,
      ContentSettingsPattern::FromString("https://firstParty/*"),
      CONTENT_SETTINGS_TYPE_PLUGINS, kCookies,
      GetDefaultAllowFromControlType(type));
  RecordShieldsSettingChanged();
}

ControlType GetCookieControlType(HostContentSettingsMap* map, const GURL& url) {
  ContentSetting setting = map->GetContentSetting(
      url, GURL(), CONTENT_SETTINGS_TYPE_PLUGINS, kCookies);

  ContentSetting fp_setting = map->GetContentSetting(
      url,
      GURL("https://firstParty/"),
      CONTENT_SETTINGS_TYPE_PLUGINS,
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
      url, GURL(), CONTENT_SETTINGS_TYPE_PLUGINS, kReferrers);

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
      CONTENT_SETTINGS_TYPE_PLUGINS, kFingerprinting,
      GetDefaultBlockFromControlType(type));

  map->SetContentSettingCustomScope(
      primary_pattern,
      ContentSettingsPattern::FromString("https://firstParty/*"),
      CONTENT_SETTINGS_TYPE_PLUGINS, kFingerprinting,
      GetDefaultAllowFromControlType(type));

  RecordShieldsSettingChanged();
}

ControlType GetFingerprintingControlType(Profile* profile, const GURL& url) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile);

  ContentSetting setting = map->GetContentSetting(
      url, GURL(), CONTENT_SETTINGS_TYPE_PLUGINS, kFingerprinting);
  ContentSetting fp_setting =
      map->GetContentSetting(url, GURL("https://firstParty/"),
                             CONTENT_SETTINGS_TYPE_PLUGINS, kFingerprinting);

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
          CONTENT_SETTINGS_TYPE_PLUGINS, kHTTPUpgradableResources,
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
          CONTENT_SETTINGS_TYPE_PLUGINS, kHTTPUpgradableResources,
          CONTENT_SETTING_DEFAULT);
}

bool GetHTTPSEverywhereEnabled(Profile* profile, const GURL& url) {
  ContentSetting setting =
      HostContentSettingsMapFactory::GetForProfile(profile)->GetContentSetting(
          url, GURL(), CONTENT_SETTINGS_TYPE_PLUGINS, kHTTPUpgradableResources);

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
          CONTENT_SETTINGS_TYPE_JAVASCRIPT, "",
          type == ControlType::ALLOW ? CONTENT_SETTING_ALLOW
                                     : CONTENT_SETTING_BLOCK);
  RecordShieldsSettingChanged();
}

ControlType GetNoScriptControlType(Profile* profile, const GURL& url) {
  ContentSetting setting =
      HostContentSettingsMapFactory::GetForProfile(profile)->GetContentSetting(
          url, GURL(), CONTENT_SETTINGS_TYPE_JAVASCRIPT, "");

  return setting == CONTENT_SETTING_ALLOW ? ControlType::ALLOW
                                          : ControlType::BLOCK;
}

bool IsAllowContentSettingFromIO(const net::URLRequest* request,
                                 const GURL& primary_url,
                                 const GURL& secondary_url,
                                 ContentSettingsType setting_type,
                                 const std::string& resource_identifier) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  //content::ResourceRequestInfo* resource_info =
  //    content::ResourceRequestInfo::ForRequest(request);
  //if (!resource_info) {
    return content_settings::GetDefaultFromResourceIdentifier(
        resource_identifier, primary_url, secondary_url) ==
            CONTENT_SETTING_ALLOW;
  //}
  //ProfileIOData* io_data =
  //    ProfileIOData::FromResourceContext(resource_info->GetContext());
  //return IsAllowContentSettingWithIOData(io_data, primary_url, secondary_url,
  //                                       setting_type, resource_identifier);
}

bool IsAllowContentSettingsForProfile(Profile* profile,
                                      const GURL& primary_url,
                                      const GURL& secondary_url,
                                      ContentSettingsType setting_type,
                                      const std::string& resource_identifier) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(profile);
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile);
  ContentSettingsForOneType settings;
  map->GetSettingsForOneType(setting_type, resource_identifier, &settings);
  return content_settings::IsAllowContentSetting(
      settings,
      primary_url,
      secondary_url,
      resource_identifier);
}

bool IsAllowContentSettingWithIOData(ProfileIOData* io_data,
                                     const GURL& primary_url,
                                     const GURL& secondary_url,
                                     ContentSettingsType setting_type,
                                     const std::string& resource_identifier) {
  if (!io_data) {
    return content_settings::GetDefaultFromResourceIdentifier(
        resource_identifier, primary_url, secondary_url);
  }

  auto* map = io_data->GetHostContentSettingsMap();
  ContentSettingsForOneType settings;
  map->GetSettingsForOneType(setting_type, resource_identifier, &settings);

  return content_settings::IsAllowContentSetting(
      settings,
      primary_url,
      secondary_url,
      resource_identifier);
}

void GetRenderFrameInfo(const URLRequest* request,
                        int* render_frame_id,
                        int* render_process_id,
                        int* frame_tree_node_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  *render_frame_id = -1;
  *render_process_id = -1;
  *frame_tree_node_id = -1;

  // PlzNavigate requests have a frame_tree_node_id, but no render_process_id
  //auto* request_info = content::ResourceRequestInfo::ForRequest(request);
  //if (request_info) {
  //  *frame_tree_node_id = request_info->GetFrameTreeNodeId();
  //}
  //if (!content::ResourceRequestInfo::GetRenderFrameForRequest(
  //        request, render_process_id, render_frame_id)) {
    content::WebSocketHandshakeRequestInfo* websocket_info =
        content::WebSocketHandshakeRequestInfo::ForRequest(request);
    if (websocket_info) {
      *render_frame_id = websocket_info->GetRenderFrameId();
      *render_process_id = websocket_info->GetChildId();
    }
  //}
}

void DispatchBlockedEventFromIO(const GURL& request_url,
                                int render_frame_id,
                                int render_process_id,
                                int frame_tree_node_id,
                                const std::string& block_type) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  base::PostTaskWithTraits(
      FROM_HERE, {BrowserThread::UI},
      base::BindOnce(&BraveShieldsWebContentsObserver::DispatchBlockedEvent,
                     block_type, request_url.spec(), render_process_id,
                     render_frame_id, frame_tree_node_id));
}

void DispatchBlockedEvent(const GURL& request_url,
                          int render_frame_id,
                          int render_process_id,
                          int frame_tree_node_id,
                          const std::string& block_type) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
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
