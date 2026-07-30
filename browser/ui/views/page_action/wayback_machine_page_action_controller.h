// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_WAYBACK_MACHINE_PAGE_ACTION_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_WAYBACK_MACHINE_PAGE_ACTION_CONTROLLER_H_

#include "base/callback_list.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wayback_machine/wayback_state.h"
#include "chrome/browser/ui/page_action/page_action_controller.h"
#include "components/tabs/public/tab_interface.h"

class ToolbarButtonProvider;

namespace actions {
class ActionItem;
}

namespace page_actions {

// Drives the Wayback Machine page action: shows an icon (badged once a
// snapshot lookup has completed) when the current page looks like it might be
// available on the Wayback Machine, and shows the Wayback Machine bubble when
// clicked.
class WaybackMachinePageActionController {
 public:
  WaybackMachinePageActionController(
      tabs::TabInterface& tab,
      page_actions::PageActionController& page_action_controller);
  WaybackMachinePageActionController(
      const WaybackMachinePageActionController&) = delete;
  WaybackMachinePageActionController& operator=(
      const WaybackMachinePageActionController&) = delete;
  ~WaybackMachinePageActionController();

  void Init();

  void ExecuteAction(ToolbarButtonProvider* toolbar_button_provider,
                     actions::ActionItem* item);

 private:
  void OnWaybackStateChanged(WaybackState state);

  // (Re-)registers for wayback-state updates on the current WebContents'
  // BraveWaybackMachineTabHelper, since the tab's contents can be swapped out
  // (e.g. tab discarding).
  void AttachToTabHelper();

  void UpdatePageAction();

  const raw_ref<tabs::TabInterface> tab_;
  const raw_ref<page_actions::PageActionControllerImpl> page_action_controller_;

  base::CallbackListSubscription did_activate_subscription_;
  base::CallbackListSubscription will_discard_contents_subscription_;

  base::WeakPtrFactory<WaybackMachinePageActionController> weak_factory_{this};
};

}  // namespace page_actions

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_WAYBACK_MACHINE_PAGE_ACTION_CONTROLLER_H_
