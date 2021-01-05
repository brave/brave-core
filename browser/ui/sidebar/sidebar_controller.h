/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_CONTROLLER_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_CONTROLLER_H_

#include <memory>
#include <string>

#include "components/prefs/pref_change_registrar.h"

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
class SidebarController {
 public:
  SidebarController(BraveBrowser* browser, Profile* profile);
  virtual ~SidebarController();

  SidebarController(const SidebarController&) = delete;
  SidebarController& operator=(const SidebarController&) = delete;

  void ActivateItemAt(int index);
  void AddItemWithCurrentTab();
  void RemoveItemAt(int index);

  bool IsActiveIndex(int index) const;

  void SetSidebar(Sidebar* sidebar);
  Sidebar* sidebar() const { return sidebar_; }

  SidebarModel* model() const { return sidebar_model_.get(); }

 private:
  void OnPreferenceChanged(const std::string& pref_name);
  void UpdateSidebarVisibility();

  BraveBrowser* browser_ = nullptr;
  // Interface to view.
  Sidebar* sidebar_ = nullptr;

  PrefChangeRegistrar profile_state_change_registrar_;
  std::unique_ptr<SidebarModel> sidebar_model_;
};

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_CONTROLLER_H_
