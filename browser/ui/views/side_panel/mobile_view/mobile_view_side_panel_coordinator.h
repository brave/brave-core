// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_MOBILE_VIEW_MOBILE_VIEW_SIDE_PANEL_COORDINATOR_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_MOBILE_VIEW_MOBILE_VIEW_SIDE_PANEL_COORDINATOR_H_

#include <memory>

#include "base/memory/raw_ref.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"
#include "url/gurl.h"

class BrowserWindowInterface;
class SidePanelRegistry;

namespace views {
class View;
}  // namespace views

// Handles the creation and registration of the mobile view panel
// SidePanelEntry.
class MobileViewSidePanelCoordinator {
 public:
  MobileViewSidePanelCoordinator(
      BrowserWindowInterface& browser_window_interface,
      const GURL& url);
  virtual ~MobileViewSidePanelCoordinator();
  MobileViewSidePanelCoordinator(const MobileViewSidePanelCoordinator&) =
      delete;
  MobileViewSidePanelCoordinator& operator=(
      const MobileViewSidePanelCoordinator&) = delete;

 private:
  std::unique_ptr<views::View> CreateView(SidePanelEntryScope& scope);
  SidePanelEntry::Key GetEntryKey() const;
  SidePanelRegistry* GetWindowRegistry();
  void DeregisterEntry();

  raw_ref<BrowserWindowInterface> browser_window_interface_;
  const GURL url_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_MOBILE_VIEW_MOBILE_VIEW_SIDE_PANEL_COORDINATOR_H_
