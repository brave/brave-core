/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"

#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "extensions/browser/extension_api_frame_id_map.h"

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

}  // namespace brave_shields
