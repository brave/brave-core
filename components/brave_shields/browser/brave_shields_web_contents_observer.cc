/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"

#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "brave/common/extensions/api/brave_shields.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_api_frame_id_map.h"


using extensions::Event;
using extensions::EventRouter;

DEFINE_WEB_CONTENTS_USER_DATA_KEY(
    brave_shields::BraveShieldsWebContentsObserver);

namespace brave_shields {

BraveShieldsWebContentsObserver::~BraveShieldsWebContentsObserver() {
}

BraveShieldsWebContentsObserver::BraveShieldsWebContentsObserver(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
}

void BraveShieldsWebContentsObserver::RenderFrameCreated(
    content::RenderFrameHost* host) {
  // Look up the extension API frame ID to force the mapping to be cached.
  // This is needed so that cached information is available for tabId.
  extensions::ExtensionApiFrameIdMap::Get()->CacheFrameData(host);
}

void BraveShieldsWebContentsObserver::DispatchBlockedEvent(
    const std::string& block_type,
    int render_process_id,
    int render_frame_id,
    int frame_tree_node_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  content::WebContents* web_contents =
      content::WebContents::FromFrameTreeNodeId(frame_tree_node_id);
  if (!web_contents) {
    content::RenderFrameHost* rfh =
        content::RenderFrameHost::FromID(render_process_id, render_frame_id);
    if (!rfh) {
      return;
    }
    web_contents =
        content::WebContents::FromRenderFrameHost(rfh);
  }

  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  EventRouter* event_router = EventRouter::Get(profile);
  if (profile && event_router) {
    extensions::api::brave_shields::OnBlocked::Details details;
    details.tab_id = extensions::ExtensionTabUtil::GetTabId(web_contents);
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

}  // namespace brave_shields
