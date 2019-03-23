/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BRAVE_SHIELDS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BRAVE_SHIELDS_UTIL_H_

#include <stdint.h>
#include <string>

#include "components/content_settings/core/common/content_settings_types.h"
#include "services/network/public/mojom/referrer_policy.mojom.h"

namespace net {
class URLRequest;
}

namespace content {
struct Referrer;
}

class GURL;
class HostContentSettingsMap;
class Profile;
class ProfileIOData;

namespace brave_shields {

bool IsAllowContentSetting(HostContentSettingsMap* content_settings,
                           const GURL& primary_url,
                           const GURL& secondary_url,
                           ContentSettingsType setting_type,
                           const std::string& resource_identifier);

bool IsAllowContentSettingWithIOData(ProfileIOData* io_data,
    const GURL& primary_url, const GURL& secondary_url,
    ContentSettingsType setting_type,
    const std::string& resource_identifier);

bool IsAllowContentSettingsForProfile(Profile* profile,
                                      const GURL& primary_url,
                                      const GURL& secondary_url,
                                      ContentSettingsType setting_type,
                                      const std::string& resource_identifier);

bool IsAllowContentSettingFromIO(const net::URLRequest* request,
    const GURL& primary_url, const GURL& secondary_url,
    ContentSettingsType setting_type,
    const std::string& resource_identifier);

void DispatchBlockedEventFromIO(const GURL &request_url, int render_frame_id,
    int render_process_id, int frame_tree_node_id,
    const std::string& block_type);

void GetRenderFrameInfo(const net::URLRequest* request,
    int* render_frame_id,
    int* render_process_id,
    int* frame_tree_node_id);

bool ShouldSetReferrer(bool allow_referrers, bool shields_up,
    const GURL& original_referrer, const GURL& tab_origin,
    const GURL& target_url, const GURL& new_referrer_url,
    network::mojom::ReferrerPolicy policy, content::Referrer *output_referrer);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BRAVE_SHIELDS_UTIL_H_
