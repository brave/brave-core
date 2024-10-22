/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"

#include <utility>

#include "brave/browser/ui/views/tabs/brave_tab_context_menu_contents.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"

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
}

const std::optional<int> BraveBrowserTabStripController::GetModelIndexOf(
    Tab* tab) {
  return tabstrip_->GetModelIndexOf(tab);
}

void BraveBrowserTabStripController::ShowContextMenuForTab(
    Tab* tab,
    const gfx::Point& p,
    ui::mojom::MenuSourceType source_type) {
  BrowserView* browser_view =
      BrowserView::GetBrowserViewForBrowser(browser());
  const auto tab_index = browser_view->tabstrip()->GetModelIndexOf(tab);
  if (!tab_index) {
    return;
  }
  context_menu_contents_ =
      std::make_unique<BraveTabContextMenuContents>(tab, this, *tab_index);
  context_menu_contents_->RunMenuAt(p, source_type);
}
