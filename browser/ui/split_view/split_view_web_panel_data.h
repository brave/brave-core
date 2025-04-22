/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SPLIT_VIEW_SPLIT_VIEW_WEB_PANEL_DATA_H_
#define BRAVE_BROWSER_UI_SPLIT_VIEW_SPLIT_VIEW_WEB_PANEL_DATA_H_

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/sidebar/sidebar_web_panel_delegate.h"

namespace content {
class WebContents;
}  // namespace content

// Manage state for showing web panel in split view.
// This class will depend on SidebarModel to do that.
class SplitViewWebPanelData : public sidebar::SidebarWebPanelDelegate {
 public:
  SplitViewWebPanelData();
  ~SplitViewWebPanelData();

  // sidebar::SidebarWebPanelDelegate:
  void OpenedForWebPanel(content::WebContents* contents) override;

  bool HasWebPanel() const;

 private:
  raw_ptr<content::WebContents> panel_contents_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_SPLIT_VIEW_SPLIT_VIEW_WEB_PANEL_DATA_H_
