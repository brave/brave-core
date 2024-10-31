/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/brave_side_panel_coordinator.h"

#include <optional>
#include <string>
#include <utility>

#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/browser/ui/views/toolbar/side_panel_button.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "brave/grit/brave_generated_resources.h"
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
    SidePanelEntry::Key entry_key,
    std::optional<SidePanelUtil::SidePanelOpenTrigger> open_trigger) {
  sidebar::SetLastUsedSidePanel(browser_view_->GetProfile()->GetPrefs(),
                                entry_key.id());

  SidePanelCoordinator::Show(entry_key, open_trigger);
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

void BraveSidePanelCoordinator::Toggle() {
  if (IsSidePanelShowing() &&
      !browser_view_->unified_side_panel()->IsClosing()) {
    Close();
  } else if (const auto key = GetLastActiveEntryKey()) {
    Show(*key, SidePanelUtil::SidePanelOpenTrigger::kToolbarButton);
  }
}

void BraveSidePanelCoordinator::Toggle(
    SidePanelEntryKey key,
    SidePanelUtil::SidePanelOpenTrigger open_trigger) {
  SidePanelCoordinator::Toggle(key, open_trigger);
}

void BraveSidePanelCoordinator::OnViewVisibilityChanged(
    views::View* observed_view,
    views::View* starting_from) {
  UpdateToolbarButtonHighlight(observed_view->GetVisible());
  SidePanelCoordinator::OnViewVisibilityChanged(observed_view, starting_from);
}

std::optional<SidePanelEntry::Key>
BraveSidePanelCoordinator::GetLastActiveEntryKey() const {
  // Don't give last active if user removed all panel items.
  const auto default_entry_id = GetDefaultEntryId(browser_view_->GetProfile());
  if (!default_entry_id) {
    return std::nullopt;
  }

  // Use last used one from previous launch instead of default entry if we have
  // it.
  if (const auto entry_id =
          sidebar::GetLastUsedSidePanel(browser_view_->browser())) {
    return SidePanelEntryKey(*entry_id);
  }

  return SidePanelEntryKey(*default_entry_id);
}

void BraveSidePanelCoordinator::UpdateToolbarButtonHighlight(
    bool side_panel_visible) {
  // Workaround to prevent crashing while window closing.
  // See https://github.com/brave/brave-browser/issues/34334
  if (!browser_view_ || !browser_view_->GetWidget() ||
      browser_view_->GetWidget()->IsClosed()) {
    return;
  }

  auto* brave_toolbar =
      static_cast<BraveToolbarView*>(browser_view_->toolbar());
  if (auto* side_panel_button = brave_toolbar->side_panel_button()) {
    side_panel_button->SetHighlighted(side_panel_visible);
    side_panel_button->SetTooltipText(l10n_util::GetStringUTF16(
        side_panel_visible ? IDS_TOOLTIP_SIDEBAR_HIDE
                           : IDS_TOOLTIP_SIDEBAR_SHOW));
  }
}

void BraveSidePanelCoordinator::PopulateSidePanel(
    bool supress_animations,
    const UniqueKey& unique_key,
    SidePanelEntry* entry,
    std::optional<std::unique_ptr<views::View>> content_view) {
  CHECK(entry);
  actions::ActionItem* const action_item = GetActionItem(entry->key());
  if (!action_item) {
    const std::string entry_id = SidePanelEntryIdToString(entry->key().id());
    LOG(ERROR) << __func__ << " no side panel action item for " << entry_id;
    SCOPED_CRASH_KEY_STRING64("SidePanel", "entry_id", entry_id);
    base::debug::DumpWithoutCrashing();
    return;
  }

  // Notify to give opportunity to observe another panel entries from
  // global or active tab's contextual registry.
  auto* brave_browser_view = static_cast<BraveBrowserView*>(browser_view_);
  CHECK(browser_view_->unified_side_panel()->children().size() == 1);
  brave_browser_view->WillShowSidePanel();
  SidePanelCoordinator::PopulateSidePanel(supress_animations, unique_key, entry,
                                          std::move(content_view));
}

void BraveSidePanelCoordinator::NotifyPinnedContainerOfActiveStateChange(
    SidePanelEntryKey key,
    bool is_active) {
  if (!browser_view_->toolbar()->pinned_toolbar_actions_container()) {
    return;
  }

  SidePanelCoordinator::NotifyPinnedContainerOfActiveStateChange(key,
                                                                 is_active);
}
