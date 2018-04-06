/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/websocket_handshake_request_info.h"
#include "extensions/browser/extension_api_frame_id_map.h"


using content::ResourceContext;
using content::BrowserThread;
using content::ResourceRequestInfo;
using net::URLRequest;

namespace brave_shields {

bool IsAllowContentSettingFromIO(net::URLRequest* request,
    GURL primary_url, GURL secondary_url, ContentSettingsType setting_type,
    const std::string& resource_identifier) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  const content::ResourceRequestInfo* resource_info =
      content::ResourceRequestInfo::ForRequest(request);
  if (!resource_info) {
    return false;
  }
  ProfileIOData* io_data =
      ProfileIOData::FromResourceContext(resource_info->GetContext());
  if (!io_data) {
    return false;
  }
  content_settings::SettingInfo setting_info;
  std::unique_ptr<base::Value> value =
      io_data->GetHostContentSettingsMap()->GetWebsiteSetting(
          primary_url, secondary_url,
          setting_type,
          resource_identifier, &setting_info);
  ContentSetting setting =
      content_settings::ValueToContentSetting(value.get());

  // TODO(bbondy): Add a static RegisterUserPrefs method for shields and use
  // prefs instead of simply returning true / false below.
  if (setting == CONTENT_SETTING_DEFAULT) {
    if (resource_identifier == "ads") {
      return false;
    } else if (resource_identifier == "trackers") {
      return false;
    } else if (resource_identifier == "httpUpgradableResources") {
      return false;
    } else if (resource_identifier == "braveShields") {
      return true;
    }
  }
  return setting == CONTENT_SETTING_ALLOW;
}

void GetRenderFrameInfo(URLRequest* request,
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
  extensions::ExtensionApiFrameIdMap::FrameData frame_data;
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

void DispatchBlockedEventFromIO(URLRequest* request,
    const std::string& block_type) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  int render_process_id, render_frame_id, frame_tree_node_id;
  GetRenderFrameInfo(request, &render_frame_id, &render_process_id,
      &frame_tree_node_id);
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
      base::BindOnce(&BraveShieldsWebContentsObserver::DispatchBlockedEvent,
          block_type, request->url().spec(),
          render_process_id, render_frame_id, frame_tree_node_id));
}

}  // namespace brave_shields
