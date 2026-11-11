// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_LINK_OPENER_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_LINK_OPENER_H_

#include <memory>

namespace blink::mojom {
class WindowFeatures;
}  // namespace blink::mojom

namespace content {
class RenderFrameHost;
class WebContents;
struct Referrer;
}  // namespace content

class BrowserWindowInterface;
class GURL;

// Opens links (e.g. `target=_blank` answer and citation links) from the AI Chat
// side panel while keeping the opening frame as the initiator. A
// destination-only browser navigation would send SameSite=Strict cookies and
// `Sec-Fetch-Site: none`.
namespace ai_chat {

// Remembers the frame that created `new_contents`. Call from
// `WebContentsObserver::DidOpenRequestedURL`, which runs before
// `AddNewContents`.
void RecordSidePanelLinkOpener(content::WebContents* new_contents,
                               content::RenderFrameHost* opener,
                               const content::Referrer& referrer);

// Opens `new_contents` in `browser`: in the active tab with the global side
// panel, otherwise as a new foreground tab. Returns the contents the link was
// opened in, or null.
content::WebContents* OpenSidePanelLinkContents(
    BrowserWindowInterface* browser,
    std::unique_ptr<content::WebContents> new_contents,
    const GURL& target_url,
    const blink::mojom::WindowFeatures& window_features,
    bool user_gesture);

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_LINK_OPENER_H_
