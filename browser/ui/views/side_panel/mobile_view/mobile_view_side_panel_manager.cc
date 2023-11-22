// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/side_panel/mobile_view/mobile_view_side_panel_manager.h"

#include <utility>

#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "chrome/browser/ui/views/side_panel/side_panel_coordinator.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry.h"

namespace {

// The user data key used to store the MobileViewSidePanelManager for a browser.
constexpr char kMobileViewSidePanelManagerKey[] =
    "mobile_view_side_panel_manager";

}  // namespace

MobileViewSidePanelManager::~MobileViewSidePanelManager() = default;

// static
MobileViewSidePanelManager* MobileViewSidePanelManager::GetOrCreateForBrowser(
    Browser* browser) {
  MobileViewSidePanelManager* manager =
      static_cast<MobileViewSidePanelManager*>(
          browser->GetUserData(kMobileViewSidePanelManagerKey));
  if (!manager) {
    // Use absl::WrapUnique(new MobileViewSidePanelManager(...)) instead of
    // std::make_unique<MobileViewSidePanelManager> to access a private
    // constructor.
    auto new_manager = absl::WrapUnique(new MobileViewSidePanelManager(
        static_cast<BraveBrowser*>(browser),
        SidePanelCoordinator::GetGlobalSidePanelRegistry(browser)));
    manager = new_manager.get();
    browser->SetUserData(kMobileViewSidePanelManagerKey,
                         std::move(new_manager));
  }
  return manager;
}

MobileViewSidePanelManager::MobileViewSidePanelManager(
    BraveBrowser* browser,
    SidePanelRegistry* global_registry)
    : browser_(*browser), global_registry_(*global_registry) {
  global_registry_observation_.Observe(&global_registry_.get());
  RegisterMobileViewSidebarItems();
}

void MobileViewSidePanelManager::RegisterMobileViewSidebarItems() {
  CHECK(browser_->sidebar_controller()->model());
  sidebar_model_observation_.Observe(browser_->sidebar_controller()->model());
}

void MobileViewSidePanelManager::CreateMobileViewSidePanelCoordinator(
    const sidebar::SidebarItem& item) {
  coordinators_.emplace(sidebar::MobileViewId(item.url.spec()),
                        std::make_unique<MobileViewSidePanelCoordinator>(
                            browser_.get(), global_registry_.get(), item.url));
}

void MobileViewSidePanelManager::OnItemAdded(const sidebar::SidebarItem& item,
                                             size_t index,
                                             bool user_gesture) {
  if (item.IsMobileViewItem()) {
    CreateMobileViewSidePanelCoordinator(item);
  }
}

void MobileViewSidePanelManager::OnWillRemoveItem(
    const sidebar::SidebarItem& item) {
  if (item.IsMobileViewItem()) {
    coordinators_.erase(sidebar::MobileViewId(item.url.spec()));
  }
}

void MobileViewSidePanelManager::OnItemUpdated(
    const sidebar::SidebarItem& item,
    const sidebar::SidebarItemUpdate& update) {
  if (item.IsMobileViewItem()) {
    // TODO(simonhong): Create coordinator for |item| if it's changed
    // as open in panel item.
    NOTIMPLEMENTED_LOG_ONCE();
  }
}

void MobileViewSidePanelManager::OnRegistryDestroying(
    SidePanelRegistry* registry) {
  coordinators_.clear();
  global_registry_observation_.Reset();
}
