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
#include "components/tabs/public/tab_interface.h"

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
  const std::optional<int> model_index = tabstrip_->GetModelIndexOf(tab);
  if (!model_index.has_value()) {
    return BrowserTabStripController::ExecuteCommandForTab(command_id, tab);
  }

  // This tab close customization targets only for split |tab|.
  // When select close tab from context menu, we want to close
  // only that split tab instead of both tabs in split.
  // Upstream closes both but we don't want that behavior.
  // As this customization could cause unexpected tab closing
  // behavior, apply strictly in some specific situations.
  // Follow upstream behavior in all other cases.
  // We can add other specific situations when user want to.
  const auto split_id = model_->GetSplitForTab(*model_index);
  if (command_id == TabStripModel::CommandCloseTab && split_id.has_value()) {
    auto* tab_interface = model_->GetTabAtIndex(*model_index);
    // If |tab| is split and selection size is 1, it means split tab
    // contains |tab| is inactive and active tab is normal. Close |tab|.
    if (model_->selection_model().size() == 1) {
      tab_interface->Close();
      return;
    }

    // If only single split tab is activated,
    // selection size is 2 as because upstream puts both tabs
    // in single split view as selected. In this situation, |tab|
    // could be in that active split view or not. Close |tab|.
    if (model_->IsActiveTabSplit() && model_->selection_model().size() == 2) {
      tab_interface->Close();
      return;
    }
  }

  BrowserTabStripController::ExecuteCommandForTab(command_id, tab);
}
