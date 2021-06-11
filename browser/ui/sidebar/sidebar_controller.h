/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_CONTROLLER_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_CONTROLLER_H_

#include <memory>
#include <string>

#include "base/scoped_observation.h"
#include "brave/components/sidebar/sidebar_service.h"

class BraveBrowser;
class GURL;
class Profile;

namespace sidebar {

class Sidebar;
class SidebarModel;

// This controls the sidebar. Each browser could have different runtime sidebar
// state and it's stored in the model. Model initializes with persisted data
// that stored in user data. That persisted data is per-profile data and
// SidebarService manages. That data will include installed sidebar item list,
// order and etc. Browser object will be the owner of this controller.
// This will observe SidebarService to know per-profile sidebar data changing
// such as adding new item or deleting existing item.
// Controller will request about add/delete items to SidebarService.
class SidebarController : public SidebarService::Observer {
 public:
  SidebarController(BraveBrowser* browser, Profile* profile);
  ~SidebarController() override;

  SidebarController(const SidebarController&) = delete;
  SidebarController& operator=(const SidebarController&) = delete;

  void ActivateItemAt(int index);
  void AddItemWithCurrentTab();
  // If current browser doesn't have a tab for |url|, active tab will load
  // |url|. Otherwise, existing tab will be activated.
  void LoadAtTab(const GURL& url);

  bool IsActiveIndex(int index) const;

  void SetSidebar(Sidebar* sidebar);
  Sidebar* sidebar() const { return sidebar_; }

  SidebarModel* model() const { return sidebar_model_.get(); }

  // SidebarService::Observer overrides:
  void OnShowSidebarOptionChanged(
      SidebarService::ShowSidebarOption option) override;

 private:
  void OnPreferenceChanged(const std::string& pref_name);
  void UpdateSidebarVisibility();

  BraveBrowser* browser_ = nullptr;
  // Interface to view.
  Sidebar* sidebar_ = nullptr;

  std::unique_ptr<SidebarModel> sidebar_model_;
  base::ScopedObservation<SidebarService, SidebarService::Observer>
      sidebar_service_observed_{this};
};

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_CONTROLLER_H_
