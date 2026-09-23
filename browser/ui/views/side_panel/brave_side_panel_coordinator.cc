/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/brave_side_panel_coordinator.h"

#include <optional>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "base/logging.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel_header.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel_header_controller.h"
#include "brave/browser/ui/views/side_panel/side_panel_utils.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"

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
    const UniqueKey& entry,
    std::optional<SidePanelOpenTrigger> open_trigger,
    bool suppress_animations) {
  sidebar::SetLastUsedSidePanel(browser_->GetProfile()->GetPrefs(),
                                entry.key.id());

  // Add item for this entry if it's hidden in sidebar but its panel is shown.
  sidebar::AddItemForSidePanelIdIfNeeded(browser_->GetProfile(),
                                         entry.key.id());

  SidePanelCoordinator::Show(entry, open_trigger, suppress_animations);

  // SidebarContainerView does not monitor panel show/hide events, so the
  // coordinator must update the active item state directly here.
  auto* controller = browser_->GetFeatures().sidebar_controller();
  CHECK(controller);
  controller->UpdateActiveItemState(
      sidebar::BuiltInItemTypeFromSidePanelId(entry.key.id()));
}

void BraveSidePanelCoordinator::Close(SidePanelEntryHideReason hide_reason,
                                      bool suppress_animations) {
  // Same as Show(): SidebarContainerView does not propagate panel close
  // events, so clear the active item state here.
  // As upstream creates SidePanelCoordinator for all browser type
  // Close() is called when Browser shutdown. When it calls from non-normal
  // browser, sidebar_controller() is null.
  if (auto* controller = browser_->GetFeatures().sidebar_controller()) {
    controller->UpdateActiveItemState();
  }

  SidePanelCoordinator::Close(hide_reason, suppress_animations);
}

void BraveSidePanelCoordinator::Toggle() {
  if (IsSidePanelShowing() && !GetSidePanel()->IsClosing()) {
    SidePanelCoordinator::Close();
  } else if (const auto key = GetLastActiveEntryKey()) {
    SidePanelUIBase::Show(*key, SidePanelOpenTrigger::kToolbarButton);
  }
}

void BraveSidePanelCoordinator::Toggle(SidePanelEntryKey key,
                                       SidePanelOpenTrigger open_trigger) {
  SidePanelCoordinator::Toggle(key, open_trigger);
}

std::optional<SidePanelEntry::Key>
BraveSidePanelCoordinator::GetLastActiveEntryKey() const {
  // Don't give last active if user removed all panel items.
  const auto default_entry_id = GetDefaultEntryId(browser_->GetProfile());
  if (!default_entry_id) {
    return std::nullopt;
  }

  // Use last used one from previous launch instead of default entry if we have
  // it.
  if (const auto entry_id = sidebar::GetLastUsedSidePanel(&*browser_)) {
    return SidePanelEntryKey(*entry_id);
  }

  return SidePanelEntryKey(*default_entry_id);
}

void BraveSidePanelCoordinator::PopulateSidePanel(
    bool supress_animations,
    const UniqueKey& unique_key,
    std::optional<SidePanelOpenTrigger> open_trigger,
    SidePanelEntry* entry,
    std::optional<std::unique_ptr<views::View>> content_view) {
  CHECK(entry);

  // Brave has its own side panel header, so hide the built-in entry headers.
  entry->set_should_show_header(false);

  actions::ActionItem* const action_item =
      SidePanelHelper::GetActionItem(&*browser_, entry->key());
  if (!action_item) {
    const std::string entry_id = SidePanelEntryIdToString(entry->key().id());
    LOG(ERROR) << __func__ << " no side panel action item for " << entry_id;
    SCOPED_CRASH_KEY_STRING64("SidePanel", "entry_id", entry_id);
    base::debug::DumpWithoutCrashing();
    return;
  }

  SidePanelCoordinator::PopulateSidePanel(supress_animations, unique_key,
                                          std::move(open_trigger), entry,
                                          std::move(content_view));

  if (brave::ShouldShowSidePanelHeader(entry->key().id())) {
    SidePanel* side_panel = GetSidePanel();
    CHECK(side_panel);
    side_panel->AddHeaderView(std::make_unique<BraveSidePanelHeader>(
        std::make_unique<BraveSidePanelHeaderController>(*browser_, entry)));
  }
}
