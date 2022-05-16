// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_BRAVE_ACTIONS_BRAVE_REWARDS_ACTION_VIEW_CONTROLLER_H_
#define BRAVE_BROWSER_UI_BRAVE_ACTIONS_BRAVE_REWARDS_ACTION_VIEW_CONTROLLER_H_

#include <memory>
#include <string>

#include "brave/browser/ui/brave_actions/brave_action_view_controller.h"
#include "extensions/common/extension_id.h"

class Browser;
class ExtensionAction;
class ExtensionRegistry;
class ExtensionsContainer;

namespace ui {
class MenuModel;
}

// The purpose of this subclass is to:
// - Add a custom context menu for the rewards extension icon. We do not want
//   to use a regular extension menu because Uninstall and Unpin extension menu
//   items do not apply to the rewards extension.
class BraveRewardsActionViewController : public BraveActionViewController {
 public:
  BraveRewardsActionViewController(const BraveRewardsActionViewController&) =
      delete;
  BraveRewardsActionViewController& operator=(
      const BraveRewardsActionViewController&) = delete;

  ~BraveRewardsActionViewController() override;

  ui::MenuModel* GetContextMenu(
      extensions::ExtensionContextMenuModel::ContextMenuSource
          context_menu_source) override;

 private:
  friend class BraveActionViewControllerFactory;
  // New instances should be instantiated with
  // BraveActionViewControllerFactory::Create().
  BraveRewardsActionViewController(
      scoped_refptr<const extensions::Extension> extension,
      Browser* browser,
      extensions::ExtensionAction* extension_action,
      extensions::ExtensionRegistry* extension_registry,
      ExtensionsContainer* extensions_container);

  std::unique_ptr<ui::MenuModel> menu_model_;
};

#endif  // BRAVE_BROWSER_UI_BRAVE_ACTIONS_BRAVE_REWARDS_ACTION_VIEW_CONTROLLER_H_
