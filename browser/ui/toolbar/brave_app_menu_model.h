/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TOOLBAR_BRAVE_APP_MENU_MODEL_H_
#define BRAVE_BROWSER_UI_TOOLBAR_BRAVE_APP_MENU_MODEL_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "brave/components/brave_vpn/buildflags/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/sidebar/buildflags/buildflags.h"
#include "chrome/browser/ui/toolbar/app_menu_model.h"
#include "ui/base/models/simple_menu_model.h"

#if BUILDFLAG(ENABLE_IPFS)
namespace ipfs {
class IpnsKeysManager;
}  // namespace ipfs
#endif

class BraveAppMenuModel : public AppMenuModel {
 public:
  BraveAppMenuModel(ui::AcceleratorProvider* provider,
                    Browser* browser,
                    AppMenuIconController* app_menu_icon_controller = nullptr);
  ~BraveAppMenuModel() override;

  BraveAppMenuModel(const BraveAppMenuModel&) = delete;
  BraveAppMenuModel& operator=(const BraveAppMenuModel&) = delete;

 private:
  // AppMenuModel overrides:
  void Build() override;
  void ExecuteCommand(int id, int event_flags) override;
  bool IsCommandIdEnabled(int id) const override;

  int GetProperItemIndex(std::vector<int> commands_to_check,
                         bool insert_next) const;

  void InsertBraveMenuItems();
  void InsertAlternateProfileItems();
  int GetIndexOfBraveRewardsItem() const;
  int GetIndexOfBraveAdBlockItem() const;
  int GetIndexOfBraveSyncItem() const;
  int GetIndexOfBraveVPNItem() const;
#if BUILDFLAG(ENABLE_SIDEBAR)
  int GetIndexOfBraveSidebarItem() const;
#endif
#if BUILDFLAG(ENABLE_IPFS)
  int FindCommandIndex(int command_id) const;
  int AddIpnsKeysToSubMenu(ui::SimpleMenuModel* submenu,
                           ipfs::IpnsKeysManager* manager,
                           int key_command_id);
  void ExecuteIPFSCommand(int id, const std::string& key);
  int AddIpfsImportMenuItem(int action_command_id,
                            int string_id,
                            int keys_command_id);
  int GetSelectedIPFSCommandId(int id) const;
  int ipns_keys_title_item_index_ = -1;
  ui::SimpleMenuModel ipfs_submenu_model_;
  std::unordered_map<int, std::unique_ptr<ui::SimpleMenuModel>>
      ipns_submenu_models_;
#endif
  std::vector<std::unique_ptr<ui::SimpleMenuModel>> sub_menus_;
};

#endif  // BRAVE_BROWSER_UI_TOOLBAR_BRAVE_APP_MENU_MODEL_H_
