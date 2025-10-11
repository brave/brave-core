/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"

#include <utility>

#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/tabs/brave_tab_context_menu_contents.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip.h"
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
  if (context_menu_contents_) {
    context_menu_contents_->Cancel();
  }
}

const std::optional<int> BraveBrowserTabStripController::GetModelIndexOf(
    Tab* tab) {
  return tabstrip_->GetModelIndexOf(tab);
}

void BraveBrowserTabStripController::EnterTabRenameModeAt(int index) {
  CHECK(base::FeatureList::IsEnabled(tabs::features::kBraveRenamingTabs));
  return static_cast<BraveTabStrip*>(tabstrip_)->EnterTabRenameModeAt(index);
}

void BraveBrowserTabStripController::SetCustomTitleForTab(
    int index,
    const std::optional<std::u16string>& title) {
  static_cast<BraveTabStripModel*>(model_.get())
      ->SetCustomTitleForTab(index, title);
}

void BraveBrowserTabStripController::ShowContextMenuForTab(
    Tab* tab,
    const gfx::Point& p,
    ui::mojom::MenuSourceType source_type) {
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  const auto tab_index = browser_view->tabstrip()->GetModelIndexOf(tab);
  if (!tab_index) {
    return;
  }
  context_menu_contents_ =
      std::make_unique<BraveTabContextMenuContents>(tab, this, *tab_index);
  context_menu_contents_->RunMenuAt(p, source_type);
}

void BraveBrowserTabStripController::ExecuteCommandForTab(
    TabStripModel::ContextMenuCommand command_id,
    const Tab* tab) {
  // TODO: Handle close tab command for closing only |tab| in split tab.
  BrowserTabStripController::ExecuteCommandForTab(command_id, tab);
}
