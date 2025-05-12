/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SPLIT_VIEW_SPLIT_VIEW_WEB_PANEL_DATA_H_
#define BRAVE_BROWSER_UI_SPLIT_VIEW_SPLIT_VIEW_WEB_PANEL_DATA_H_

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/sidebar/sidebar_web_panel_delegate.h"
#include "components/tab_collections/public/tab_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class Browser;
class GURL;
class SplitViewView;

namespace content {
class WebContents;
}  // namespace content

// Manage state for showing web panel in split view.
// This class will depend on SidebarModel to do that.
class SplitViewWebPanelData : public sidebar::SidebarWebPanelDelegate,
                              public TabStripModelObserver {
 public:
  SplitViewWebPanelData();
  ~SplitViewWebPanelData() override;

  // sidebar::SidebarWebPanelDelegate:
  void LoadInWebPanel(const sidebar::SidebarItem& item) override;

  // TabStripModelObserver:
  void OnTabWillBeRemoved(content::WebContents* contents, int index) override;

  void SetBrowser(Browser* browser);
  content::WebContents* GetActivePanelContents() const;
  void UpdateActivePanelContents(content::WebContents* new_active_contents);
  bool HasActiveWebPanel() const;
  int GetSizeDelta();

  raw_ptr<SplitViewView> view_ = nullptr;

 private:
  base::flat_map<GURL, tabs::TabInterface*> tab_for_web_panel_item_;
  raw_ptr<tabs::TabInterface> tab_for_active_web_panel_ = nullptr;
  raw_ptr<Browser> browser_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_SPLIT_VIEW_SPLIT_VIEW_WEB_PANEL_DATA_H_
