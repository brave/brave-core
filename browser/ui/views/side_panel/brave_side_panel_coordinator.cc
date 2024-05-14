/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/brave_side_panel_coordinator.h"

#include <optional>

#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"

namespace {

std::optional<SidePanelEntry::Id> GetDefaultEntryId(Profile* profile) {
  auto* service = sidebar::SidebarServiceFactory::GetForProfile(profile);
  auto panel_item = service->GetDefaultPanelItem();
  if (panel_item.has_value()) {
    return SidePanelIdFromSideBarItem(panel_item.value());
  }
  return std::nullopt;
}

}  // namespace

BraveSidePanelCoordinator::~BraveSidePanelCoordinator() = default;

void BraveSidePanelCoordinator::Show(
    std::optional<SidePanelEntry::Id> entry_id,
    std::optional<SidePanelUtil::SidePanelOpenTrigger> open_trigger) {
  // If user clicks sidebar toolbar button, |entry_id| is null.
  // Then, choose Brave's own default, and exclude items that user has removed
  // from sidebar. If none are enabled, do nothing.
  auto* profile = browser_view_->GetProfile();
  if (!entry_id) {
    // Early return as user removes all default panel entries.
    auto default_entry_id = GetDefaultEntryId(profile);
    if (!default_entry_id.has_value()) {
      return;
    }
    entry_id = sidebar::GetLastUsedSidePanel(browser_view_->browser());
    if (!entry_id.has_value()) {
      // Use default pick when we don't have lastly used panel.
      entry_id = default_entry_id;
    }
  }

  // Cache lastly shown entry id to make it persist across the re-launch.
  if (entry_id) {
    sidebar::SetLastUsedSidePanel(browser_view_->GetProfile()->GetPrefs(),
                                  *entry_id);
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

void BraveSidePanelCoordinator::UpdateToolbarButtonHighlight(
    bool side_panel_visible) {
  // Workaround to prevent crashing while window closing.
  // See https://github.com/brave/brave-browser/issues/34334
  if (!browser_view_ || !browser_view_->GetWidget() ||
      browser_view_->GetWidget()->IsClosed()) {
    return;
  }

  SidePanelCoordinator::UpdateToolbarButtonHighlight(side_panel_visible);
}
