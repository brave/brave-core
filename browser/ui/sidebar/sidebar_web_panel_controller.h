/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_WEB_PANEL_CONTROLLER_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_WEB_PANEL_CONTROLLER_H_

#include "base/memory/raw_ref.h"

class BrowserView;
class BraveMultiContentsView;

namespace sidebar {

struct SidebarItem;

class SidebarWebPanelController {
 public:
  explicit SidebarWebPanelController(BrowserView& browser);
  ~SidebarWebPanelController();

  SidebarWebPanelController(const SidebarWebPanelController&) = delete;
  SidebarWebPanelController& operator=(const SidebarWebPanelController&) =
      delete;

  void OpenWebPanel(const SidebarItem& item);
  void CloseWebPanel();
  bool IsShowingWebPanel() const;

 private:
  BraveMultiContentsView* GetMultiContentsView();
  const BraveMultiContentsView* GetMultiContentsView() const;

  raw_ref<BrowserView> browser_view_;
};

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_WEB_PANEL_CONTROLLER_H_
