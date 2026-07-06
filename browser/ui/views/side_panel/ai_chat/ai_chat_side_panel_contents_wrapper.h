// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_CONTENTS_WRAPPER_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_CONTENTS_WRAPPER_H_

#include "base/functional/callback.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_ui.h"
#include "chrome/browser/ui/webui/top_chrome/webui_contents_wrapper.h"
#include "url/gurl.h"

class Profile;

namespace content {
class WebContents;
}  // namespace content

// WebUIContentsWrapper for the AI Chat side panel that forwards link-hover
// target URL changes to a callback. Unlike a normal browser tab, the side
// panel's WebContents delegate does not drive the browser status bubble, so
// hovering a link would otherwise show nothing. The hosting view uses the
// forwarded URL to display its own status bubble, mirroring the bottom-left
// URL preview shown for normal tabs.
class AIChatSidePanelContentsWrapper : public WebUIContentsWrapperT<AIChatUI> {
 public:
  AIChatSidePanelContentsWrapper(const GURL& webui_url,
                                 Profile* profile,
                                 int task_manager_string_id,
                                 bool esc_closes_ui);
  ~AIChatSidePanelContentsWrapper() override;

  // Set the callback invoked whenever the hovered link URL changes. An empty
  // URL means no link is hovered.
  void SetTargetURLChangedCallback(
      base::RepeatingCallback<void(const GURL&)> callback);

  // content::WebContentsDelegate:
  void UpdateTargetURL(content::WebContents* source, const GURL& url) override;

 private:
  base::RepeatingCallback<void(const GURL&)> target_url_changed_callback_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_CONTENTS_WRAPPER_H_
