/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_MOBILE_VIEW_MOBILE_VIEW_SIDE_PANEL_MANAGER_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_MOBILE_VIEW_MOBILE_VIEW_SIDE_PANEL_MANAGER_H_

#include <memory>

#include "base/containers/flat_map.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ref.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/components/sidebar/browser/mobile_view_id.h"

class MobileViewSidePanelCoordinator;
class SidePanelRegistry;

namespace sidebar {
FORWARD_DECLARE_TEST(SidebarBrowserTestWithMobileViewFeature,
                     SimpleItemAddRemoveTest);
}  // namespace sidebar

// Manage each mobile view panel item's coordinator by observing SidebarModel.
// Create/Remove it with item add/remove state.
class MobileViewSidePanelManager : public sidebar::SidebarModel::Observer {
 public:
  explicit MobileViewSidePanelManager(SidePanelRegistry& window_registry);

  MobileViewSidePanelManager(const MobileViewSidePanelManager&) = delete;
  MobileViewSidePanelManager& operator=(const MobileViewSidePanelManager&) =
      delete;
  ~MobileViewSidePanelManager() override;

  void Init(sidebar::SidebarModel* model);

 private:
  FRIEND_TEST_ALL_PREFIXES(sidebar::SidebarBrowserTestWithMobileViewFeature,
                           SimpleItemAddRemoveTest);

  // SidebarModel::Observer overrides:
  void OnItemAdded(const sidebar::SidebarItem& item,
                   size_t index,
                   bool user_gesture) override;
  void OnWillRemoveItem(const sidebar::SidebarItem& item) override;

  void CreateMobileViewSidePanelCoordinator(const sidebar::SidebarItem& item);

  base::flat_map<sidebar::MobileViewId,
                 std::unique_ptr<MobileViewSidePanelCoordinator>>
      coordinators_;
  base::raw_ref<SidePanelRegistry> window_registry_;
  base::ScopedObservation<sidebar::SidebarModel,
                          sidebar::SidebarModel::Observer>
      sidebar_model_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_MOBILE_VIEW_MOBILE_VIEW_SIDE_PANEL_MANAGER_H_
