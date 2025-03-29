/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/mobile_view/mobile_view_side_panel_manager.h"

#include <utility>

#include "brave/browser/ui/views/side_panel/mobile_view/mobile_view_side_panel_coordinator.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "ui/views/view.h"

MobileViewSidePanelManager::MobileViewSidePanelManager(
    BrowserWindowInterface& browser_window_interface)
    : browser_window_interface_(browser_window_interface) {}

MobileViewSidePanelManager::~MobileViewSidePanelManager() = default;

void MobileViewSidePanelManager::Init(sidebar::SidebarModel* model) {
  CHECK(model);
  sidebar_model_observation_.Observe(model);
}

void MobileViewSidePanelManager::CreateMobileViewSidePanelCoordinator(
    const sidebar::SidebarItem& item) {
  coordinators_.emplace(sidebar::MobileViewId(item.url.spec()),
                        std::make_unique<MobileViewSidePanelCoordinator>(
                            browser_window_interface_.get(), item.url));
}

void MobileViewSidePanelManager::OnItemAdded(const sidebar::SidebarItem& item,
                                             size_t index,
                                             bool user_gesture) {
  if (item.IsMobileViewItem()) {
    CHECK(!coordinators_.contains(sidebar::MobileViewId(item.url.spec())));
    CreateMobileViewSidePanelCoordinator(item);
  }
}

void MobileViewSidePanelManager::OnWillRemoveItem(
    const sidebar::SidebarItem& item) {
  if (item.IsMobileViewItem()) {
    CHECK(coordinators_.contains(sidebar::MobileViewId(item.url.spec())));
    coordinators_.erase(sidebar::MobileViewId(item.url.spec()));
  }
}
