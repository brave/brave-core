// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_PARTITIONED_STORAGE_PAGE_ACTION_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_PARTITIONED_STORAGE_PAGE_ACTION_CONTROLLER_H_

#include <memory>

#include "base/callback_list.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/views/page_action/page_action_controller.h"
#include "components/tabs/public/tab_interface.h"

namespace actions {
class ActionItem;
}

namespace brave {
class ContainersTabMenuModelDelegate;
}

namespace containers {
class ContainersMenuModel;
}

namespace views {
class MenuModelAdapter;
class MenuRunner;
}  // namespace views

namespace page_actions {

// Drives the Partitioned Storage page action (container indicator): shows the
// action with container icon/name when the tab is in a Brave container, hides
// it otherwise.
class PartitionedStoragePageActionController {
 public:
  PartitionedStoragePageActionController(
      tabs::TabInterface& tab,
      page_actions::PageActionController& page_action_controller);
  PartitionedStoragePageActionController(
      const PartitionedStoragePageActionController&) = delete;
  PartitionedStoragePageActionController& operator=(
      const PartitionedStoragePageActionController&) = delete;
  ~PartitionedStoragePageActionController();

  void Init();

  void ExecuteAction(actions::ActionItem* item);

 private:
  void UpdatePageAction();
  void OnPartitionedStorageMenuClosed();

  const raw_ref<tabs::TabInterface> tab_;
  const raw_ref<page_actions::PageActionController> page_action_controller_;
  base::CallbackListSubscription did_activate_subscription_;
  base::CallbackListSubscription did_become_visible_subscription_;
  base::CallbackListSubscription will_discard_contents_subscription_;

  std::unique_ptr<brave::ContainersTabMenuModelDelegate>
      containers_menu_delegate_;
  std::unique_ptr<containers::ContainersMenuModel> containers_menu_model_;
  std::unique_ptr<views::MenuModelAdapter> menu_model_adapter_;
  std::unique_ptr<views::MenuRunner> menu_runner_;
  raw_ptr<actions::ActionItem> action_item_for_menu_ = nullptr;

  base::WeakPtrFactory<PartitionedStoragePageActionController>
      weak_ptr_factory_{this};
};

}  // namespace page_actions

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_PARTITIONED_STORAGE_PAGE_ACTION_CONTROLLER_H_
