// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_PSST_ACTION_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_PSST_ACTION_CONTROLLER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/psst/psst_menu_model.h"
#include "chrome/browser/ui/page_actions/page_action_controller.h"
#include "components/tabs/public/tab_interface.h"
#include "ui/views/controls/menu/menu_model_adapter.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/view_tracker.h"

class ToolbarButtonProvider;

namespace page_actions {

class PsstActionController {
 public:
  PsstActionController(
      tabs::TabInterface& tab,
      page_actions::PageActionController& page_action_controller);
  PsstActionController(const PsstActionController&) = delete;
  PsstActionController& operator=(const PsstActionController&) = delete;
  ~PsstActionController();

  void SetMenuModelObserver(psst::PsstMenuModel::Observer* observer);
  void SetVisible(bool visible);
  void SetShowBadge(bool show);

  base::WeakPtr<PsstActionController> AsWeakPtr();

  void ExecuteAction(ToolbarButtonProvider* toolbar_button_provider,
                     actions::ActionItem* item);

 private:
  void UpdatePageAction();
  void OnPsstMenuClosed();

  bool show_badge_ = false;

  const raw_ref<tabs::TabInterface> tab_;
  const raw_ref<page_actions::PageActionControllerImpl> page_action_controller_;
  base::CallbackListSubscription did_activate_subscription_;

  raw_ptr<psst::PsstMenuModel::Observer> psst_menu_model_observer_ = nullptr;
  std::unique_ptr<psst::PsstMenuModel> psst_menu_model_;
  std::unique_ptr<views::MenuModelAdapter> menu_model_adapter_;
  std::unique_ptr<views::MenuRunner> menu_runner_;
  raw_ptr<actions::ActionItem> action_item_for_menu_ = nullptr;

  // Clears if the page action `View` is destroyed while a menu is open
  views::ViewTracker menu_anchor_view_tracker_;

  base::WeakPtrFactory<PsstActionController> weak_ptr_factory_{this};
};

}  // namespace page_actions

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_PSST_ACTION_CONTROLLER_H_
