/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/brave_side_panel_coordinator.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/types/to_address.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel_header.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel_header_controller.h"
#include "brave/browser/ui/views/side_panel/side_panel_utils.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_news/common/buildflags/buildflags.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_coordinator.h"
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ui/side_panel/ai_chat/ai_chat_side_panel_utils.h"
#include "brave/browser/ui/views/side_panel/ai_chat/ai_chat_side_panel_web_view.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
#include "base/feature_list.h"
#include "brave/browser/ui/views/side_panel/brave_news/brave_news_side_panel_web_view.h"
#include "brave/components/brave_news/common/features.h"
#endif

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

BraveSidePanelCoordinator::BraveSidePanelCoordinator(
    BrowserWindowInterface* browser)
    : SidePanelCoordinator(browser) {
  [[maybe_unused]] SidePanelRegistry* global_registry =
      SidePanelRegistry::From(browser);

#if BUILDFLAG(ENABLE_PLAYLIST)
  // The playlist coordinator is not created for popup windows, or for
  // desktop PWAs.
  if (auto* playlist_coordinator =
          PlaylistSidePanelCoordinator::From(browser)) {
    playlist_coordinator->CreateAndRegisterEntry(global_registry);
  }
#endif  // BUILDFLAG(ENABLE_PLAYLIST)

#if BUILDFLAG(ENABLE_AI_CHAT)
  // AI Chat side panel as a global panel and not tab-specific is conditional
  // for now.
  // TODO(https://github.com/brave/brave-browser/issues/48526): Remove the
  // condition when the feature flag is removed.
  if (ai_chat::AIChatServiceFactory::GetForBrowserContext(
          browser->GetProfile()) &&
      ai_chat::ShouldSidePanelBeGlobal(browser->GetProfile())) {
    global_registry->Register(std::make_unique<SidePanelEntry>(
        SidePanelEntry::Key(SidePanelEntry::Id::kChatUI),
        base::BindRepeating(&AIChatSidePanelWebView::CreateView,
                            browser->GetProfile(),
                            /*is_tab_associated=*/false),
        base::NullCallback()));
  }
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
  if (base::FeatureList::IsEnabled(brave_news::features::kBraveNewsSidebar)) {
    global_registry->Register(std::make_unique<SidePanelEntry>(
        SidePanelEntry::Key(SidePanelEntry::Id::kBraveNews),
        base::BindRepeating(&BraveNewsSidePanelWebView::CreateView,
                            browser->GetProfile()),
        base::NullCallback()));
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_NEWS)
}

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
  auto* controller =
      sidebar::SidebarController::From(base::to_address(browser_));
  CHECK(controller);
  controller->HandleSidePanelOpened(entry.key.id());
}

void BraveSidePanelCoordinator::Close(SidePanelEntryHideReason hide_reason,
                                      bool suppress_animations) {
  // Same as Show(): SidebarContainerView does not propagate panel close
  // events, so clear the active item state here.
  // As upstream creates SidePanelCoordinator for all browser type
  // Close() is called when Browser shutdown. When it calls from non-normal
  // browser, there is no SidebarController.
  if (auto* controller =
          sidebar::SidebarController::From(base::to_address(browser_))) {
    controller->HandleSidePanelClosed();
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
