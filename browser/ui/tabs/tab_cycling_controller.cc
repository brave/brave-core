/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/public/tab_cycling_controller.h"

#include "base/check_deref.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "ui/base/base_window.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/views/event_monitor.h"

DEFINE_USER_DATA(TabCyclingController);

TabCyclingController::TabCyclingController(BrowserWindowInterface& browser)
    : browser_(browser),
      scoped_unowned_user_data_(browser.GetUnownedUserDataHost(), *this) {
  browser.GetTabStripModel()->AddObserver(this);
}

TabCyclingController::~TabCyclingController() = default;

// static
TabCyclingController* TabCyclingController::From(
    BrowserWindowInterface* browser) {
  return Get(browser->GetUnownedUserDataHost());
}

void TabCyclingController::Start() {
  const gfx::NativeWindow native_window =
      browser_->GetWindow()->GetNativeWindow();
  monitor_ = views::EventMonitor::CreateWindowMonitor(
      this, native_window,
      {ui::EventType::kMousePressed, ui::EventType::kKeyReleased});
  widget_observation_.Observe(
      &CHECK_DEREF(views::Widget::GetWidgetForNativeWindow(native_window)));
}

void TabCyclingController::Stop() {
  if (!is_cycling()) {
    return;
  }

  monitor_.reset();
  widget_observation_.Reset();
  static_cast<BraveTabStripModel*>(browser_->GetTabStripModel())
      ->StopMRUCycling();
}

void TabCyclingController::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  // A tab can be closed while cycling, e.g. with Ctrl+F4, after which the next
  // Ctrl+Tab starts a new cycle.
  if (change.type() != TabStripModelChange::kSelectionOnly) {
    Stop();
  }
}

void TabCyclingController::OnEvent(const ui::Event& event) {
  if (event.type() == ui::EventType::kMousePressed ||
      (event.type() == ui::EventType::kKeyReleased &&
       event.AsKeyEvent()->key_code() == ui::VKEY_CONTROL)) {
    Stop();
  }
}

void TabCyclingController::OnWidgetActivationChanged(views::Widget* widget,
                                                     bool active) {
  if (!active) {
    Stop();
  }
}

void TabCyclingController::OnWidgetClosing(views::Widget* widget) {
  Stop();
}
