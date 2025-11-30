// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_WEB_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_WEB_VIEW_H_

#include <memory>

#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"

class AIChatUI;
class Profile;

// A custom web view to set focus correctly when the side panel is shown.
class AIChatSidePanelWebView : public SidePanelWebUIViewT<AIChatUI> {
 public:
  // Factory method to create and configure an AIChatSidePanelWebView.
  // If `is_tab_associated` is true, the side panel will be related
  // to the active tab and will change conversation when the tab navigates.
  static std::unique_ptr<views::View> CreateView(Profile* profile,
                                                 bool is_tab_associated,
                                                 SidePanelEntryScope& scope);

  AIChatSidePanelWebView(
      SidePanelEntryScope& scope,
      std::unique_ptr<WebUIContentsWrapperT<AIChatUI>> contents_wrapper);
  ~AIChatSidePanelWebView() override;

  // Disable copy and assign.
  AIChatSidePanelWebView(const AIChatSidePanelWebView&) = delete;
  AIChatSidePanelWebView& operator=(const AIChatSidePanelWebView&) = delete;

  // WebUIContentsWrapper::Host:
  content::WebContents* AddNewContents(
      content::WebContents* source,
      std::unique_ptr<content::WebContents> new_contents,
      const GURL& target_url,
      WindowOpenDisposition disposition,
      const blink::mojom::WindowFeatures& window_features,
      bool user_gesture,
      bool* was_blocked) override;

 private:
  // This callback is invoked multiple times, so we need to ensure that
  // focus is set only once with `should_focus_`.
  void OnShow();

  // Whether focus should be set when the side panel is shown. We only do this
  // for the first time the side panel is shown, and not for subsequent shows.
  bool should_focus_ = true;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_WEB_VIEW_H_
