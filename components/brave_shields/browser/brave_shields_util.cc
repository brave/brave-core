/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/extensions/api/brave_shields.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/websocket_handshake_request_info.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_api_frame_id_map.h"


using extensions::Event;
using extensions::EventRouter;

namespace brave_shields {

bool IsAllowContentSetting(content::ResourceContext* resource_context,
    GURL tab_origin, ContentSettingsType setting_type) {
  content_settings::SettingInfo info;
  ProfileIOData* io_data = ProfileIOData::FromResourceContext(
      resource_context);
  std::unique_ptr<base::Value> value =
      io_data->GetHostContentSettingsMap()->GetWebsiteSetting(
          tab_origin, tab_origin,
          setting_type,
          std::string(), &info);
  ContentSetting setting =
      content_settings::ValueToContentSetting(value.get());
  return setting == CONTENT_SETTING_ALLOW;
}

void GetRenderFrameIdAndProcessId(net::URLRequest* request,
    int* render_frame_id,
    int* render_process_id) {
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

int GetTabId(net::URLRequest* request) {
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

void DispatchBlockedEvent(const std::string &block_type,
    net::URLRequest* request) {
  Profile* profile = ProfileManager::GetActiveUserProfile();
  EventRouter* event_router = EventRouter::Get(profile);
  if (profile && event_router) {
    extensions::api::brave_shields::OnBlocked::Details details;
    details.tab_id = GetTabId(request);
    details.block_type = block_type;
    std::unique_ptr<base::ListValue> args(
        extensions::api::brave_shields::OnBlocked::Create(details)
          .release());
    std::unique_ptr<Event> event(
        new Event(extensions::events::BRAVE_AD_BLOCKED,
          extensions::api::brave_shields::OnBlocked::kEventName,
          std::move(args)));
    event_router->BroadcastEvent(std::move(event));
  }
}
// true if able to perform the lookup.  The URL is stored to *url, and
// *is_incognito is set to indicate whether the URL is for an incognito tab.
bool GetUrlForTabId(int tab_id,
                    GURL* url) {
  Profile* profile = ProfileManager::GetActiveUserProfile();
  content::WebContents* contents = NULL;
  Browser* browser = NULL;
  bool found = extensions::ExtensionTabUtil::GetTabById(
      tab_id,
      profile,
      true,  // Search incognito tabs, too.
      &browser,
      NULL,
      &contents,
      NULL);

  if (found) {
    *url = contents->GetURL();
    return true;
  } else {
    return false;
  }
}

}  // namespace brave_shields
