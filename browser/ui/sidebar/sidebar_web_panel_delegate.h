/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_WEB_PANEL_DELEGATE_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_WEB_PANEL_DELEGATE_H_

namespace sidebar {
struct SidebarItem;

class SidebarWebPanelDelegate {
 public:
  virtual void LoadInWebPanel(const SidebarItem& item) = 0;

 protected:
  ~SidebarWebPanelDelegate() = default;
};

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_WEB_PANEL_DELEGATE_H_
