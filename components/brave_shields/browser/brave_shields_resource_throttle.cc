/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_shields_resource_throttle.h"

#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/tracking_protection_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "brave/common/extensions/api/brave_shields.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/websocket_handshake_request_info.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_api_frame_id_map.h"
#include "extensions/browser/extension_event_histogram_value.h"
#include "net/url_request/url_request.h"

using extensions::EventRouter;
using extensions::Event;

namespace {

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
  GURL url = request->initiator()->GetURL();
  Profile* profile = ProfileManager::GetActiveUserProfile();
  EventRouter* event_router = EventRouter::Get(profile);
  if (profile && event_router) {
    extensions::api::brave_shields::OnBlocked::Details details;
    details.url = url.spec();
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

}  // namespace

content::ResourceThrottle* MaybeCreateBraveShieldsResourceThrottle(
    net::URLRequest* request,
    content::ResourceContext* resource_context,
    content::ResourceType resource_type) {
  return new BraveShieldsResourceThrottle(request, resource_context,
      resource_type);
}

BraveShieldsResourceThrottle::BraveShieldsResourceThrottle(
    net::URLRequest* request,
    content::ResourceContext* resource_context,
    content::ResourceType resource_type) :
      request_(request),
      resource_context_(resource_context),
      resource_type_(resource_type) {
}

BraveShieldsResourceThrottle::~BraveShieldsResourceThrottle() = default;

const char* BraveShieldsResourceThrottle::GetNameForLogging() const {
  return "BraveShieldsResourceThrottle";
}

void BraveShieldsResourceThrottle::WillStartRequest(bool* defer) {

  content_settings::SettingInfo info;
  ProfileIOData* io_data = ProfileIOData::FromResourceContext(
      resource_context_);
  std::unique_ptr<base::Value> adBlockValue =
      io_data->GetHostContentSettingsMap()->GetWebsiteSetting(
          request_->initiator()->GetURL(), request_->initiator()->GetURL(),
          CONTENT_SETTINGS_TYPE_BRAVEADBLOCK,
          std::string(), &info);
  ContentSetting adBlockSetting =
      content_settings::ValueToContentSetting(adBlockValue.get());
  bool allow_ad_block = adBlockSetting == CONTENT_SETTING_ALLOW;

  std::unique_ptr<base::Value> trackingProtectionValue =
      io_data->GetHostContentSettingsMap()->GetWebsiteSetting(
          request_->initiator()->GetURL(), request_->initiator()->GetURL(),
          CONTENT_SETTINGS_TYPE_BRAVETRACKINGPROTECTION,
          std::string(), &info);
  ContentSetting trackingProtectionSetting =
      content_settings::ValueToContentSetting(trackingProtectionValue.get());
  bool allow_tracking_protection =
    trackingProtectionSetting == CONTENT_SETTING_ALLOW;

  if (allow_ad_block && !g_browser_process->tracking_protection_service()->
      ShouldStartRequest(request_->url(),
      resource_type_,
      request_->initiator()->host())) {
    Cancel();
    DispatchBlockedEvent("adBlock", request_);
  }
  if (allow_tracking_protection && !g_browser_process->ad_block_service()->
      ShouldStartRequest(request_->url(),
      resource_type_,
      request_->initiator()->host())) {
    Cancel();
    DispatchBlockedEvent("trackingProtection", request_);
  }
}
