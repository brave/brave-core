// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_PSST_ACTION_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_PSST_ACTION_CONTROLLER_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list_types.h"
#include "chrome/browser/ui/page_actions/page_action_controller.h"
#include "components/tabs/public/tab_interface.h"
#include "ui/menus/simple_menu_model.h"
#include "ui/views/controls/menu/menu_model_adapter.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/view_tracker.h"

class ToolbarButtonProvider;

namespace page_actions {

class PsstActionController : public ui::SimpleMenuModel::Delegate {
 public:
  // Observes selections made in the PSST page action menu.
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnDontShowThisSiteSelected() = 0;
    virtual void OnDisablePrivacySettingsTuningSelected() = 0;

   protected:
    ~Observer() override = default;
  };

  PsstActionController(
      tabs::TabInterface& tab,
      page_actions::PageActionController& page_action_controller);
  PsstActionController(const PsstActionController&) = delete;
  PsstActionController& operator=(const PsstActionController&) = delete;
  ~PsstActionController() override;

  void SetMenuModelObserver(Observer* observer);
  void SetVisible(bool visible);
  void SetShowBadge(bool show);

  base::WeakPtr<PsstActionController> AsWeakPtr();

  void ExecuteAction(ToolbarButtonProvider* toolbar_button_provider,
                     actions::ActionItem* item);

 private:
  // ui::SimpleMenuModel::Delegate:
  void ExecuteCommand(int command_id, int event_flags) override;
  bool IsCommandIdEnabled(int command_id) const override;

  void BuildMenuItems();
  void UpdatePageAction();
  void OnPsstMenuClosed();

  bool show_badge_ = false;

  const raw_ref<tabs::TabInterface> tab_;
  const raw_ref<page_actions::PageActionControllerImpl> page_action_controller_;
  base::CallbackListSubscription did_activate_subscription_;

  std::unique_ptr<ui::SimpleMenuModel> psst_menu_model_;
  std::unique_ptr<views::MenuModelAdapter> menu_model_adapter_;
  std::unique_ptr<views::MenuRunner> menu_runner_;

  // not owned
  raw_ptr<Observer> psst_menu_model_observer_ = nullptr;
  raw_ptr<actions::ActionItem> action_item_for_menu_ = nullptr;

  // Clears if the page action `View` is destroyed while a menu is open
  views::ViewTracker menu_anchor_view_tracker_;

  base::WeakPtrFactory<PsstActionController> weak_ptr_factory_{this};
};

}  // namespace page_actions

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_PSST_ACTION_CONTROLLER_H_
