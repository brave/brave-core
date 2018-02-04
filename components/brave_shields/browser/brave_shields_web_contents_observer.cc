/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_shields/browser/brave_shields_stats.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "brave/common/extensions/api/brave_shields.h"
#include "brave/common/render_messages.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_api_frame_id_map.h"
#include "ipc/ipc_message_macros.h"


using extensions::Event;
using extensions::EventRouter;
using content::RenderFrameHost;

DEFINE_WEB_CONTENTS_USER_DATA_KEY(
    brave_shields::BraveShieldsWebContentsObserver);

namespace {

content::WebContents* GetWebContents(
    int render_process_id,
    int render_frame_id,
    int frame_tree_node_id) {
  content::WebContents* web_contents =
      content::WebContents::FromFrameTreeNodeId(frame_tree_node_id);
  if (!web_contents) {
    RenderFrameHost* rfh =
        RenderFrameHost::FromID(render_process_id, render_frame_id);
    if (!rfh) {
      return nullptr;
    }
    web_contents =
        content::WebContents::FromRenderFrameHost(rfh);
  }
  return web_contents;
}

}  // namespace


namespace brave_shields {

BraveShieldsWebContentsObserver::~BraveShieldsWebContentsObserver() {
}

BraveShieldsWebContentsObserver::BraveShieldsWebContentsObserver(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
}

void BraveShieldsWebContentsObserver::RenderFrameCreated(
    RenderFrameHost* host) {
  // Look up the extension API frame ID to force the mapping to be cached.
  // This is needed so that cached information is available for tabId.
  extensions::ExtensionApiFrameIdMap::Get()->CacheFrameData(host);
}

void BraveShieldsWebContentsObserver::DispatchBlockedEvent(
    const std::string& block_type,
    const std::string& subresource,
    int render_process_id,
    int render_frame_id,
    int frame_tree_node_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  content::WebContents* web_contents = GetWebContents(render_process_id,
    render_frame_id, frame_tree_node_id);
  DispatchBlockedEventForWebContents(block_type, subresource, web_contents);

  auto* stats = BraveShieldsStats::GetInstance();
  if (block_type == kAds) {
    stats->IncrementAdsBlocked();
  } else if (block_type == kTrackers) {
    stats->IncrementTrackersBlocked();
  } else if (block_type == kHTTPUpgradableResources) {
    stats->IncrementHttpsUpgrades();
  } else if (block_type == kJavaScript) {
    stats->IncrementJavascriptBlocked();
  } else if (block_type == kFingerprinting) {
    stats->IncrementFingerprintingBlocked();
  }
}

void BraveShieldsWebContentsObserver::DispatchBlockedEventForWebContents(
    const std::string& block_type, const std::string& subresource,
    content::WebContents* web_contents) {
  if (!web_contents) {
    return;
  }
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  EventRouter* event_router = EventRouter::Get(profile);
  if (profile && event_router) {
    extensions::api::brave_shields::OnBlocked::Details details;
    details.tab_id = extensions::ExtensionTabUtil::GetTabId(web_contents);
    details.block_type = block_type;
    details.subresource = subresource;
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

bool BraveShieldsWebContentsObserver::OnMessageReceived(
    const IPC::Message& message, RenderFrameHost* render_frame_host) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(BraveShieldsWebContentsObserver,
        message, render_frame_host)
    IPC_MESSAGE_HANDLER(BraveViewHostMsg_JavaScriptBlocked,
        OnJavaScriptBlockedWithDetail)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void BraveShieldsWebContentsObserver::OnJavaScriptBlockedWithDetail(
    RenderFrameHost* render_frame_host,
    const base::string16& details) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents) {
    return;
  }
  DispatchBlockedEventForWebContents(brave_shields::kJavaScript,
      base::UTF16ToUTF8(details), web_contents);
}

}  // namespace brave_shields
