/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_WEB_PANEL_CONTROLLER_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_WEB_PANEL_CONTROLLER_H_

#include "base/memory/raw_ref.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class BrowserView;
class BraveMultiContentsView;

namespace content {
class WebContents;
}  // namespace content

namespace sidebar {

class SidebarWebPanelController : public TabStripModelObserver {
 public:
  explicit SidebarWebPanelController(BrowserView& browser);
  ~SidebarWebPanelController() override;

  SidebarWebPanelController(const SidebarWebPanelController&) = delete;
  SidebarWebPanelController& operator=(const SidebarWebPanelController&) =
      delete;

  void ToggleWebPanel(const SidebarItem& item);

  const content::WebContents* panel_contents() const {
    return panel_contents_.get();
  }

 private:
  BraveMultiContentsView* GetMultiContentsView();
  const BraveMultiContentsView* GetMultiContentsView() const;

  void OpenWebPanel(const SidebarItem& item);
  void CloseWebPanel();
  bool IsShowingWebPanel() const;

  // TabStripModelObesrver:
  void OnTabWillBeRemoved(content::WebContents* contents, int index) override;

  raw_ref<BrowserView> browser_view_;
  raw_ptr<content::WebContents> panel_contents_ = nullptr;
  sidebar::SidebarItem panel_item_;
};

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_WEB_PANEL_CONTROLLER_H_
