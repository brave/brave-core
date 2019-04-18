/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_shields_util.h"

#include <memory>

#include "base/task/post_task.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/shield_exceptions.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/browser/referrer_whitelist_service.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/websocket_handshake_request_info.h"
#include "content/public/common/referrer.h"
#include "extensions/browser/extension_api_frame_id_map.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

using content::BrowserThread;
using content::Referrer;
using content::ResourceContext;
using content::ResourceRequestInfo;
using net::URLRequest;

namespace brave_shields {

namespace {

bool GetDefaultFromResourceIdentifier(const std::string& resource_identifier,
                                      const GURL& primary_url,
                                      const GURL& secondary_url) {
  if (resource_identifier == brave_shields::kAds) {
    return false;
  } else if (resource_identifier == brave_shields::kTrackers) {
    return false;
  } else if (resource_identifier == brave_shields::kHTTPUpgradableResources) {
    return false;
  } else if (resource_identifier == brave_shields::kBraveShields) {
    return true;
  } else if (resource_identifier == brave_shields::kReferrers) {
    return false;
  } else if (resource_identifier == brave_shields::kCookies) {
    return secondary_url == GURL("https://firstParty/");
  }
  return false;
}

}  // namespace

bool IsAllowContentSetting(HostContentSettingsMap* content_settings,
                           const GURL& primary_url,
                           const GURL& secondary_url,
                           ContentSettingsType setting_type,
                           const std::string& resource_identifier) {
  DCHECK(content_settings);
  content_settings::SettingInfo setting_info;
  std::unique_ptr<base::Value> value = content_settings->GetWebsiteSetting(
      primary_url, secondary_url, setting_type, resource_identifier,
      &setting_info);
  ContentSetting setting = content_settings::ValueToContentSetting(value.get());

  // TODO(bbondy): Add a static RegisterUserPrefs method for shields and use
  // prefs instead of simply returning true / false below.
  if (setting == CONTENT_SETTING_DEFAULT) {
    return GetDefaultFromResourceIdentifier(resource_identifier, primary_url,
                                            secondary_url);
  }
  return setting == CONTENT_SETTING_ALLOW;
}

bool IsAllowContentSettingFromIO(const net::URLRequest* request,
                                 const GURL& primary_url,
                                 const GURL& secondary_url,
                                 ContentSettingsType setting_type,
                                 const std::string& resource_identifier) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  content::ResourceRequestInfo* resource_info =
      content::ResourceRequestInfo::ForRequest(request);
  if (!resource_info) {
    return GetDefaultFromResourceIdentifier(resource_identifier, primary_url,
                                            secondary_url);
  }
  ProfileIOData* io_data =
      ProfileIOData::FromResourceContext(resource_info->GetContext());
  return IsAllowContentSettingWithIOData(io_data, primary_url, secondary_url,
                                         setting_type, resource_identifier);
}

bool IsAllowContentSettingsForProfile(Profile* profile,
                                      const GURL& primary_url,
                                      const GURL& secondary_url,
                                      ContentSettingsType setting_type,
                                      const std::string& resource_identifier) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(profile);
  return IsAllowContentSetting(
      HostContentSettingsMapFactory::GetForProfile(profile), primary_url,
      secondary_url, setting_type, resource_identifier);
}

bool IsAllowContentSettingWithIOData(ProfileIOData* io_data,
                                     const GURL& primary_url,
                                     const GURL& secondary_url,
                                     ContentSettingsType setting_type,
                                     const std::string& resource_identifier) {
  if (!io_data) {
    return GetDefaultFromResourceIdentifier(resource_identifier, primary_url,
                                            secondary_url);
  }
  return IsAllowContentSetting(io_data->GetHostContentSettingsMap(),
                               primary_url, secondary_url, setting_type,
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
  auto* request_info = content::ResourceRequestInfo::ForRequest(request);
  if (request_info) {
    *frame_tree_node_id = request_info->GetFrameTreeNodeId();
  }
  if (!content::ResourceRequestInfo::GetRenderFrameForRequest(
          request, render_process_id, render_frame_id)) {
    const content::WebSocketHandshakeRequestInfo* websocket_info =
        content::WebSocketHandshakeRequestInfo::ForRequest(request);
    if (websocket_info) {
      *render_frame_id = websocket_info->GetRenderFrameId();
      *render_process_id = websocket_info->GetChildId();
    }
  }
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
