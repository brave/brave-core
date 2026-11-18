// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_SHIELDS_AD_BLOCK_POPUP_UTIL_H_
#define BRAVE_BROWSER_BRAVE_SHIELDS_AD_BLOCK_POPUP_UTIL_H_

#include <optional>

#include "base/functional/callback_forward.h"
#include "content/public/browser/global_routing_id.h"
#include "url/origin.h"

class GURL;
class HostContentSettingsMap;

namespace content {
class RenderFrameHost;
}  // namespace content

namespace brave_shields {

class AdBlockService;

struct PopupOpener {
  url::Origin source;
  bool aggressive = false;
  content::GlobalRenderFrameHostToken frame_token;
};

// Returns the opener to check a popup against, or nullopt if Shields or ad
// blocking is off for the opener's tab or `url` is the link the user pressed.
// Settings come from `opener_main_frame`, and `source` is the frame that
// opened the window.
std::optional<PopupOpener> GetPopupOpener(
    HostContentSettingsMap* content_settings,
    content::RenderFrameHost* opener_main_frame,
    const url::Origin& source,
    const GURL& url);

// Whether a popup to `url` should be checked at all.
bool ShouldCheckPopupUrl(HostContentSettingsMap* content_settings,
                         const GURL& url);

// Runs `callback` asynchronously with whether `url` matches a `$popup` rule,
// and counts a match as a blocked ad on the opener's tab.
void ShouldBlockPopup(AdBlockService* ad_block_service,
                      const PopupOpener& opener,
                      const GURL& url,
                      base::OnceCallback<void(bool)> callback);

}  // namespace brave_shields

#endif  // BRAVE_BROWSER_BRAVE_SHIELDS_AD_BLOCK_POPUP_UTIL_H_
