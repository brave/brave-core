/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_actions/brave_rewards_action_view_controller.h"

#include <memory>
#include <utility>

#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/extensions/extension_context_menu_model.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/extensions/extensions_container.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_action.h"
#include "extensions/browser/extension_action_manager.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "ui/base/models/simple_menu_model.h"

namespace {

class BraveRewardsActionMenuModel : public ui::SimpleMenuModel,
                                    public ui::SimpleMenuModel::Delegate {
 public:
  explicit BraveRewardsActionMenuModel(PrefService* prefs)
      : SimpleMenuModel(this), prefs_(prefs) {
    Build();
  }

  ~BraveRewardsActionMenuModel() override = default;
  BraveRewardsActionMenuModel(const BraveRewardsActionMenuModel&) = delete;
  BraveRewardsActionMenuModel& operator=(const BraveRewardsActionMenuModel&) =
      delete;

 private:
  enum ContextMenuCommand {
    HideBraveRewardsIcon,
  };

  // ui::SimpleMenuModel::Delegate override:
  void ExecuteCommand(int command_id, int event_flags) override {
    if (command_id == HideBraveRewardsIcon)
      prefs_->SetBoolean(brave_rewards::prefs::kShowButton, false);
  }

  void Build() {
    AddItemWithStringId(HideBraveRewardsIcon,
                        IDS_HIDE_BRAVE_REWARDS_ACTION_ICON);
  }

  raw_ptr<PrefService> prefs_ = nullptr;
};

}  // namespace

BraveRewardsActionViewController::BraveRewardsActionViewController(
    scoped_refptr<const extensions::Extension> extension,
    Browser* browser,
    extensions::ExtensionAction* extension_action,
    extensions::ExtensionRegistry* extension_registry,
    ExtensionsContainer* extensions_container)
    : BraveActionViewController(std::move(extension),
                                browser,
                                extension_action,
                                extension_registry,
                                extensions_container),
      menu_model_(std::make_unique<BraveRewardsActionMenuModel>(
          browser->profile()->GetPrefs())) {}

BraveRewardsActionViewController::~BraveRewardsActionViewController() = default;

ui::MenuModel* BraveRewardsActionViewController::GetContextMenu(
    extensions::ExtensionContextMenuModel::ContextMenuSource
        context_menu_source) {
  return menu_model_.get();
}
