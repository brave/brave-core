/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/page_action/wayback_machine_state_manager.h"

#include "base/functional/bind.h"
#include "brave/browser/ui/views/page_action/wayback_machine_action_icon_view.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"
#include "brave/components/brave_wayback_machine/wayback_state.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

WaybackMachineStateManager::WaybackMachineStateManager(
    WaybackMachineActionIconView* icon,
    Browser* browser)
    : icon_(*icon), browser_(*browser) {
  CHECK(browser);
  browser_->tab_strip_model()->AddObserver(this);
}

WaybackMachineStateManager::~WaybackMachineStateManager() = default;

WaybackState WaybackMachineStateManager::GetActiveTabWaybackState() const {
  auto* active_contents = browser_->tab_strip_model()->GetActiveWebContents();
  if (!active_contents) {
    return WaybackState::kInitial;
  }

  auto* tab_helper =
      BraveWaybackMachineTabHelper::FromWebContents(active_contents);
  CHECK(tab_helper);
  return tab_helper->wayback_state();
}

void WaybackMachineStateManager::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (!selection.active_tab_changed()) {
    return;
  }

  if (selection.old_contents) {
    auto* tab_helper =
        BraveWaybackMachineTabHelper::FromWebContents(selection.old_contents);
    CHECK(tab_helper);
    tab_helper->SetWaybackStateChangedCallback(base::NullCallback());

    // Try to close if old tab had bubble.
    if (auto* widget = views::Widget::GetWidgetForNativeWindow(
            tab_helper->active_window())) {
      widget->CloseWithReason(views::Widget::ClosedReason::kUnspecified);
      tab_helper->set_active_window(nullptr);
    }
  }

  if (selection.new_contents) {
    auto* tab_helper =
        BraveWaybackMachineTabHelper::FromWebContents(selection.new_contents);
    CHECK(tab_helper);
    tab_helper->SetWaybackStateChangedCallback(
        base::BindRepeating(&WaybackMachineStateManager::OnWaybackStateChanged,
                            weak_factory_.GetWeakPtr()));
  }
}

void WaybackMachineStateManager::OnWaybackStateChanged(WaybackState state) {
  icon_->Update();
}
