// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"
#include "chrome/grit/generated_resources.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace {

absl::optional<SidePanelEntry::Id> GetDefaultEntryId(Profile* profile) {
  auto* service = sidebar::SidebarServiceFactory::GetForProfile(profile);
  auto panel_item = service->GetDefaultPanelItem();
  if (panel_item.has_value()) {
    return SidePanelIdFromSideBarItem(panel_item.value());
  }
  return absl::nullopt;
}

}  // namespace

// Undef upstream's to avoid redefined error.
#undef IDS_TOOLTIP_SIDE_PANEL_HIDE
#undef IDS_TOOLTIP_SIDE_PANEL_SHOW

#define IDS_TOOLTIP_SIDE_PANEL_HIDE IDS_TOOLTIP_SIDEBAR_HIDE
#define IDS_TOOLTIP_SIDE_PANEL_SHOW IDS_TOOLTIP_SIDEBAR_SHOW

// Brave has its own side panel navigation in the form of the SideBar, so
// hide the Chromium combobox-style header.
#define BRAVE_SIDE_PANEL_COORDINATOR_CREATE_HEADER header->SetVisible(false);

// Choose Brave's own default, and exclude items that user has removed
// from sidebar. If none are enabled, do nothing.
#define BRAVE_SIDE_PANEL_COORDINATOR_SHOW                      \
  if (!entry_id.has_value() && !GetLastActiveEntryKey()) {     \
    entry_id = GetDefaultEntryId(browser_view_->GetProfile()); \
    if (!entry_id.has_value()) {                               \
      return;                                                  \
    }                                                          \
  }

// Undef upstream's to avoid redefined error.
#undef IDS_TOOLTIP_SIDE_PANEL_HIDE
#undef IDS_TOOLTIP_SIDE_PANEL_SHOW

#define IDS_TOOLTIP_SIDE_PANEL_HIDE IDS_TOOLTIP_SIDEBAR_HIDE
#define IDS_TOOLTIP_SIDE_PANEL_SHOW IDS_TOOLTIP_SIDEBAR_SHOW

#define SidePanel BraveSidePanel
#include "src/chrome/browser/ui/views/side_panel/side_panel_coordinator.cc"
#undef SidePanel
#undef IDS_TOOLTIP_SIDE_PANEL_HIDE
#undef IDS_TOOLTIP_SIDE_PANEL_SHOW
#undef BRAVE_SIDE_PANEL_COORDINATOR_CREATE_HEADER
#undef BRAVE_SIDE_PANEL_COORDINATOR_SHOW
