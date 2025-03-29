// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/side_panel/mobile_view/mobile_view_side_panel_coordinator.h"

#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/views/side_panel/side_panel_coordinator.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry.h"
#include "url/gurl.h"

MobileViewSidePanelCoordinator::MobileViewSidePanelCoordinator(
    BrowserWindowInterface& browser_window_interface,
    const GURL& url)
    : browser_window_interface_(browser_window_interface), url_(url) {
  CHECK(url_.is_valid());

  // Using Unretained() here is safe becuase this coordinator
  // will be destroyed by MobileViewSidePanelManager
  // while SidePanelCoordinator() lives. It owns this registry.
  GetWindowRegistry()->Register(std::make_unique<SidePanelEntry>(
      GetEntryKey(),
      base::BindRepeating(&MobileViewSidePanelCoordinator::CreateView,
                          base::Unretained(this))));
}

MobileViewSidePanelCoordinator::~MobileViewSidePanelCoordinator() {
  DeregisterEntry();
}

std::unique_ptr<views::View> MobileViewSidePanelCoordinator::CreateView(
    SidePanelEntryScope& scope) {
  // TODO(simonhong): Implement UI.
  NOTREACHED();
}

SidePanelEntry::Key MobileViewSidePanelCoordinator::GetEntryKey() const {
  return SidePanelEntry::Key(SidePanelEntry::Id::kMobileView,
                             sidebar::MobileViewId(url_.spec()));
}

void MobileViewSidePanelCoordinator::DeregisterEntry() {
  GetWindowRegistry()->Deregister(GetEntryKey());
}

SidePanelRegistry* MobileViewSidePanelCoordinator::GetWindowRegistry() {
  return browser_window_interface_->GetFeatures()
      .side_panel_coordinator()
      ->GetWindowRegistry();
}
