/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"

#include <utility>

#include "brave/browser/ui/views/tabs/brave_tab_context_menu_contents.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "ui/views/widget/widget.h"

#if defined(USE_AURA)
#include "ui/aura/window.h"
#endif

BraveBrowserTabStripController::BraveBrowserTabStripController(
    TabStripModel* model,
    BrowserView* browser_view,
    std::unique_ptr<TabMenuModelFactory> menu_model_factory_override)
    : BrowserTabStripController(model,
                                browser_view,
                                std::move(menu_model_factory_override)) {}

BraveBrowserTabStripController::~BraveBrowserTabStripController() {
  if (context_menu_contents_)
    context_menu_contents_->Cancel();

  if (ctrl_released_event_handler_.get()) {
    // We are still MRU cycling so ctrl realase handler is still registered
    ui::EventHandler::DisableCheckTargets();
  }
}

void BraveBrowserTabStripController::ShowContextMenuForTab(
    Tab* tab,
    const gfx::Point& p,
    ui::MenuSourceType source_type) {
  BrowserView* browser_view =
      BrowserView::GetBrowserViewForBrowser(browser());
  context_menu_contents_ = std::make_unique<BraveTabContextMenuContents>(
      tab,
      this,
      browser_view->tabstrip()->GetModelIndexOf(tab));
  context_menu_contents_->RunMenuAt(p, source_type);
}

void BraveBrowserTabStripController::StartMRUCycling(
    BraveTabStripModel* brave_tab_strip_model) {
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  ctrl_released_event_handler_ =
      std::make_unique<CtrlReleaseHandler>(brave_tab_strip_model, browser_view);
  views::Widget* widget = browser_view->tabstrip()->GetWidget();

#if defined(OS_MACOSX)
  if (widget)
    widget->GetRootView()->AddPreTargetHandler(
        ctrl_released_event_handler_.get());
#else
  if (widget)
    widget->GetNativeWindow()->AddPreTargetHandler(
        ctrl_released_event_handler_.get());
#endif
}

BraveBrowserTabStripController::CtrlReleaseHandler::CtrlReleaseHandler(
    BraveTabStripModel* model,
    BrowserView* browser_view)
    : model_(model), browser_view_(browser_view) {}

BraveBrowserTabStripController::CtrlReleaseHandler::~CtrlReleaseHandler() =
    default

void BraveBrowserTabStripController::CtrlReleaseHandler::OnKeyEvent(
    ui::KeyEvent* event) {
  if (event->key_code() == ui::VKEY_CONTROL &&
      event->type() == ui::ET_KEY_RELEASED) {
    // Ctrl key was released, stop the MRU cycling

    // Remove event handler
    views::Widget* widget = browser_view_->tabstrip()->GetWidget();
#if defined(OS_MACOSX)
    if (widget)
      widget->GetRootView()->RemovePreTargetHandler(this);
#else
    if (widget)
      widget->GetNativeWindow()->RemovePreTargetHandler(this);
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
