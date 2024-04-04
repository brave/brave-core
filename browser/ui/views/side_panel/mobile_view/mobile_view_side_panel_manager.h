// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_MOBILE_VIEW_MOBILE_VIEW_SIDE_PANEL_MANAGER_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_MOBILE_VIEW_MOBILE_VIEW_SIDE_PANEL_MANAGER_H_

#include <memory>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ref.h"
#include "base/scoped_observation.h"
#include "base/supports_user_data.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/views/side_panel/mobile_view/mobile_view_side_panel_coordinator.h"
#include "brave/components/sidebar/mobile_view_id.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry_observer.h"

class BraveBrowser;
class Browser;
class SidePanelRegistry;

class MobileViewSidePanelManager : public base::SupportsUserData::Data,
                                   public sidebar::SidebarModel::Observer,
                                   public SidePanelRegistryObserver {
 public:
  MobileViewSidePanelManager(const MobileViewSidePanelManager&) = delete;
  MobileViewSidePanelManager& operator=(const MobileViewSidePanelManager&) =
      delete;
  ~MobileViewSidePanelManager() override;

  static MobileViewSidePanelManager* GetOrCreateForBrowser(Browser* browser);

 private:
  MobileViewSidePanelManager(BraveBrowser* browser,
                             SidePanelRegistry* registry);

  // SidebarModel::Observer overrides:
  void OnItemAdded(const sidebar::SidebarItem& item,
                   size_t index,
                   bool user_gesture) override;
  void OnWillRemoveItem(const sidebar::SidebarItem& item) override;
  void OnItemUpdated(const sidebar::SidebarItem& item,
                     const sidebar::SidebarItemUpdate& update) override;

  // SidePanelRegistryObserver overrides:
  void OnRegistryDestroying(SidePanelRegistry* registry) override;

  void RegisterMobileViewSidebarItems();
  void CreateMobileViewSidePanelCoordinator(const sidebar::SidebarItem& item);

  base::flat_map<sidebar::MobileViewId,
                 std::unique_ptr<MobileViewSidePanelCoordinator>>
      coordinators_;
  raw_ref<BraveBrowser> browser_;
  raw_ref<SidePanelRegistry> global_registry_;
  base::ScopedObservation<sidebar::SidebarModel,
                          sidebar::SidebarModel::Observer>
      sidebar_model_observation_{this};
  base::ScopedObservation<SidePanelRegistry, SidePanelRegistryObserver>
      global_registry_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_MOBILE_VIEW_MOBILE_VIEW_SIDE_PANEL_MANAGER_H_
