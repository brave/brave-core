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
    GURL tab_origin, ContentSettingsType setting_type,
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
          tab_origin, tab_origin,
          setting_type,
          resource_identifier, &setting_info);
  ContentSetting setting =
      content_settings::ValueToContentSetting(value.get());
  return setting != CONTENT_SETTING_BLOCK;
}

void GetRenderFrameIdAndProcessId(URLRequest* request,
    int* render_frame_id,
    int* render_process_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  *render_frame_id = -1;
  *render_process_id = -1;
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

int GetTabIdFromIO(URLRequest* request) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  int render_frame_id = -1;
  int render_process_id = -1;
  int tab_id = -1;
  GetRenderFrameIdAndProcessId(request, &render_frame_id, &render_process_id);
  extensions::ExtensionApiFrameIdMap::FrameData frame_data;
  if (extensions::ExtensionApiFrameIdMap::Get()->GetCachedFrameDataOnIO(
      render_process_id, render_frame_id, &frame_data)) {
    tab_id = frame_data.tab_id;
  }
  return tab_id;
}

void DispatchBlockedEventFromIO(URLRequest* request,
    const std::string& block_type) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  int render_process_id, render_frame_id;
  GetRenderFrameIdAndProcessId(request, &render_frame_id, &render_process_id);
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
      base::BindOnce(&BraveShieldsWebContentsObserver::DispatchBlockedEvent,
          block_type, render_process_id, render_frame_id));
}

}  // namespace brave_shields
