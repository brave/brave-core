// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/side_panel/mobile_view/mobile_view_side_panel_coordinator.h"

#include "chrome/browser/ui/views/side_panel/side_panel_registry.h"
#include "url/gurl.h"

MobileViewSidePanelCoordinator::MobileViewSidePanelCoordinator(
    SidePanelRegistry& window_registry,
    const GURL& url)
    : window_registry_(window_registry), url_(url) {
  CHECK(url_.is_valid());

  // Using Unretained() here is safe becuase this coordinator
  // will be destroyed by MobileViewSidePanelManager
  // while SidePanelCoordinator() lives. It owns this registry.
  window_registry_->Register(std::make_unique<SidePanelEntry>(
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
  // https://github.com/brave/brave-browser/issues/34597
  NOTREACHED();
}

SidePanelEntry::Key MobileViewSidePanelCoordinator::GetEntryKey() const {
  return SidePanelEntry::Key(SidePanelEntry::Id::kMobileView,
                             sidebar::MobileViewId(url_.spec()));
}

void MobileViewSidePanelCoordinator::DeregisterEntry() {
  window_registry_->Deregister(GetEntryKey());
}
