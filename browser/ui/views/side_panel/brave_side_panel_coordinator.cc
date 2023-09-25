/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/brave_side_panel_coordinator.h"

#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"

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

BraveSidePanelCoordinator::~BraveSidePanelCoordinator() = default;

void BraveSidePanelCoordinator::Show(
    absl::optional<SidePanelEntry::Id> entry_id,
    absl::optional<SidePanelUtil::SidePanelOpenTrigger> open_trigger) {
  // Choose Brave's own default, and exclude items that user has removed
  // from sidebar. If none are enabled, do nothing.
  if (!entry_id.has_value() && !GetLastActiveEntryKey()) {
    entry_id = GetDefaultEntryId(browser_view_->GetProfile());
    if (!entry_id.has_value()) {
      return;
    }
  }

  SidePanelCoordinator::Show(entry_id, open_trigger);
}

void BraveSidePanelCoordinator::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  auto* brave_browser_view = static_cast<BraveBrowserView*>(browser_view_);
  const bool active_tab_changed = selection.active_tab_changed();
  if (active_tab_changed) {
    brave_browser_view->SetSidePanelOperationByActiveTabChange(true);
  }

  SidePanelCoordinator::OnTabStripModelChanged(tab_strip_model, change,
                                               selection);

  // Clear as this flag is only used for show/hide operation triggered by above
  // SidePanelCoordinator::OnTabStripModelChanged().
  if (active_tab_changed) {
    brave_browser_view->SetSidePanelOperationByActiveTabChange(false);
  }
}

std::unique_ptr<views::View> BraveSidePanelCoordinator::CreateHeader() {
  auto header = SidePanelCoordinator::CreateHeader();

  // Brave has its own side panel navigation in the form of the SideBar, so
  // hide the Chromium combobox-style header.
  header->SetVisible(false);
  return header;
}
