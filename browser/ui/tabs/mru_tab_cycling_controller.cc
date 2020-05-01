/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/mru_tab_cycling_controller.h"

#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "content/public/browser/web_contents.h"
#include "ui/events/event.h"
#include "ui/events/event_handler.h"

#if defined(USE_AURA)
#include "ui/aura/window.h"
#endif

MRUTabCyclingController::MRUTabCyclingController(
    BraveTabStripModel* brave_tab_strip_model)
    : model_(brave_tab_strip_model) {}

MRUTabCyclingController::~MRUTabCyclingController() {
  if (ctrl_released_event_handler_.get()) {
    // We are still MRU cycling so ctrl realase handler is still registered
    ui::EventHandler::DisableCheckTargets();
  }
}

void MRUTabCyclingController::StartMRUCycling() {
  ctrl_released_event_handler_ = std::make_unique<CtrlReleaseHandler>(model_);

  // Add the event handler
  gfx::NativeWindow window =
      model_->GetActiveWebContents()->GetTopLevelNativeWindow();
#if defined(OS_MACOSX)
  views::Widget::GetWidgetForNativeWindow(window)
      ->GetRootView()
      ->AddPreTargetHandler(ctrl_released_event_handler_.get());
#else
  window->AddPreTargetHandler(ctrl_released_event_handler_.get());
#endif
}

MRUTabCyclingController::CtrlReleaseHandler::CtrlReleaseHandler(
    BraveTabStripModel* model)
    : model_(model) {}

MRUTabCyclingController::CtrlReleaseHandler::~CtrlReleaseHandler() = default;

void MRUTabCyclingController::CtrlReleaseHandler::OnKeyEvent(
    ui::KeyEvent* event) {
  if (event->key_code() == ui::VKEY_CONTROL &&
      event->type() == ui::ET_KEY_RELEASED) {
    // Ctrl key was released, stop the MRU cycling

    // Remove event handler
    gfx::NativeWindow window =
        model_->GetActiveWebContents()->GetTopLevelNativeWindow();
#if defined(OS_MACOSX)
    views::Widget::GetWidgetForNativeWindow(window)
        ->GetRootView()
        ->RemovePreTargetHandler(this);
#else
    window->RemovePreTargetHandler(this);
#endif

    model_->StopMRUCycling();

  } else if (!((event->key_code() == ui::VKEY_TAB &&
                event->type() == ui::ET_KEY_PRESSED) ||
               (event->key_code() == ui::VKEY_PRIOR &&
                event->type() == ui::ET_KEY_PRESSED) ||
               (event->key_code() == ui::VKEY_NEXT &&
                event->type() == ui::ET_KEY_PRESSED))) {
    // Block all keys while cycling except tab,pg previous, pg next keys
    event->StopPropagation();
  }
}
