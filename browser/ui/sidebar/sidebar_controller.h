/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_CONTROLLER_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_CONTROLLER_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/sidebar/sidebar_item.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/window_open_disposition.h"

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
class SidebarController : public SidebarService::Observer,
                          public TabStripModelObserver {
 public:
  SidebarController(BraveBrowser* browser, Profile* profile);
  ~SidebarController() override;

  SidebarController(const SidebarController&) = delete;
  SidebarController& operator=(const SidebarController&) = delete;

  // |disposition| is only valid for shortcut type. If |disposition| is not
  // CURRENT_TAB, item at |index| is handled based on |disposition|.
  void ActivateItemAt(
      absl::optional<size_t> index,
      WindowOpenDisposition disposition = WindowOpenDisposition::CURRENT_TAB);
  void AddItemWithCurrentTab();
  // If current browser doesn't have a tab for |url|, active tab will load
  // |url|. Otherwise, existing tab will be activated.
  // ShowSingletonTab() has similar functionality but it loads url in the
  // new tab.
  void LoadAtTab(const GURL& url);

  bool IsActiveIndex(absl::optional<size_t> index) const;

  bool DoesBrowserHaveOpenedTabForItem(const SidebarItem& item) const;

  void SetSidebar(Sidebar* sidebar);
  Sidebar* sidebar() const { return sidebar_; }

  SidebarModel* model() const { return sidebar_model_.get(); }

  // SidebarService::Observer overrides:
  void OnShowSidebarOptionChanged(
      SidebarService::ShowSidebarOption option) override;

  // TabStripModelObserver
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

 private:
  void OnPreferenceChanged(const std::string& pref_name);
  void UpdateSidebarVisibility();

  // Iterate tabs by host (if tabs with host of URL exist).
  // Otherwise, load URL in the active tab.
  void IterateOrLoadAtActiveTab(const GURL& url);

  // Try to find a tab that loads |url| from other browsers
  // and activate it if found.
  bool ActiveTabFromOtherBrowsersForHost(const GURL& url);

  raw_ptr<BraveBrowser> browser_ = nullptr;
  // Interface to view.
  raw_ptr<Sidebar> sidebar_ = nullptr;
  // If there is a tab-specific panel open, this is the type to restore
  // when changing active tab to a tab without a tab-specific panel open.
  absl::optional<SidebarItem::BuiltInItemType> browser_active_panel_type_ =
      absl::nullopt;

  std::unique_ptr<SidebarModel> sidebar_model_;
  base::ScopedObservation<SidebarService, SidebarService::Observer>
      sidebar_service_observed_{this};
};

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_CONTROLLER_H_
