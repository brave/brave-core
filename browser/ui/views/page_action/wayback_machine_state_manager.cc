/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/page_action/wayback_machine_state_manager.h"

#include <optional>

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
    std::optional<gfx::NativeWindow> active_window =
        tab_helper->active_window();
    if (active_window.has_value()) {
      if (auto* widget =
              views::Widget::GetWidgetForNativeWindow(active_window.value())) {
        widget->CloseWithReason(views::Widget::ClosedReason::kUnspecified);
        tab_helper->set_active_window(std::nullopt);
      }
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

void WaybackMachineStateManager::OnTabGroupChanged(
    const TabGroupChange& change) {
  if (change.type != TabGroupChange::kCreated ||
      change.GetCreateChange()->reason() !=
          TabGroupChange::TabGroupCreationReason::
              kInsertedFromAnotherTabstrip) {
    return;
  }

  auto* model = browser_->tab_strip_model();
  const int active_index = model->active_index();
  if (model->empty() || active_index == TabStripModel::kNoTab) {
    return;
  }

  // Why we have to find previous active web contents and reset callback here?
  // We clear callback when it becomes inactive tab via
  // OnTabStripModelChanged(). However, it doesn't work as expected when active
  // tab is changed by tab group re-attaching. When it's re-attached, new tab
  // from tab group is activated but |selection.old_contents| is null when
  // OnTabStripModelChanged(). Curious why it's null. I think it should point to
  // previous active web contents.
  const int tab_count = model->count();
  for (int i = 0; i < tab_count; ++i) {
    if (i == active_index) {
      continue;
    }

    auto* web_contents = model->GetWebContentsAt(i);
    auto* tab_helper =
        BraveWaybackMachineTabHelper::FromWebContents(web_contents);
    CHECK(tab_helper);
    tab_helper->SetWaybackStateChangedCallback(base::NullCallback());
  }
}

void WaybackMachineStateManager::OnWaybackStateChanged(WaybackState state) {
  icon_->Update();
}
