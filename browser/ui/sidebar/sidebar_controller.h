/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_CONTROLLER_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_CONTROLLER_H_

#include <memory>
#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
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
class SidebarController : public SidebarService::Observer {
 public:
  SidebarController(BraveBrowser* browser, Profile* profile);
  ~SidebarController() override;

  SidebarController(const SidebarController&) = delete;
  SidebarController& operator=(const SidebarController&) = delete;

  // NOTE: Don't call this directly for panel item. Use ActivatePanelItem().
  // This should be called as a result of SidePanelCoordinator's entry
  // opening/closing event. If this method is called directly for activating
  // panel, SidePanelCoordinator doesn't know about it.

  // |disposition| is only valid for shortcut type. If |disposition| is not
  // CURRENT_TAB, item at |index| is handled based on |disposition|.
  void ActivateItemAt(
      std::optional<size_t> index,
      WindowOpenDisposition disposition = WindowOpenDisposition::CURRENT_TAB);
  void AddItemWithCurrentTab();
  void UpdateActiveItemState(std::optional<SidebarItem::BuiltInItemType>
                                 active_panel_item = std::nullopt);

  // Ask panel item activation state change to SidePanelUI.
  void ActivatePanelItem(SidebarItem::BuiltInItemType panel_item);
  void DeactivateCurrentPanel();

  // If current browser doesn't have a tab for |url|, active tab will load
  // |url|. Otherwise, existing tab will be activated.
  // ShowSingletonTab() has similar functionality but it loads url in the
  // new tab.
  void LoadAtTab(const GURL& url);

  bool IsActiveIndex(std::optional<size_t> index) const;
  bool DoesBrowserHaveOpenedTabForItem(const SidebarItem& item) const;

  void SetSidebar(Sidebar* sidebar);
  Sidebar* sidebar() const { return sidebar_; }

  SidebarModel* model() const { return sidebar_model_.get(); }

  // SidebarService::Observer overrides:
  void OnShowSidebarOptionChanged(
      SidebarService::ShowSidebarOption option) override;

 private:
  void OnPreferenceChanged(const std::string& pref_name);

  // Iterate tabs by host (if tabs with host of URL exist).
  // Otherwise, load URL in the active tab.
  void IterateOrLoadAtActiveTab(const GURL& url);

  // Try to find a tab that loads |url| from other browsers
  // and activate it if found.
  bool ActiveTabFromOtherBrowsersForHost(const GURL& url);

  raw_ptr<BraveBrowser> browser_ = nullptr;
  // Interface to view.
  raw_ptr<Sidebar, DanglingUntriaged> sidebar_ = nullptr;

  std::unique_ptr<SidebarModel> sidebar_model_;
  base::ScopedObservation<SidebarService, SidebarService::Observer>
      sidebar_service_observed_{this};
};

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_CONTROLLER_H_
