/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_context_menu_contents.h"

#include <algorithm>
#include <iterator>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/notimplemented.h"
#include "base/notreached.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_tab_menu_model.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_muted_utils.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/tabs/tab_utils.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "content/public/browser/web_contents.h"
#include "ui/compositor/compositor.h"
#include "ui/views/controls/menu/menu_runner.h"

BraveTabContextMenuContents::BraveTabContextMenuContents(
    Tab* tab,
    BraveBrowserTabStripController* controller,
    int index)
    : tab_(tab),
      tab_index_(index),
      browser_(const_cast<Browser*>(controller->browser())),
      controller_(controller) {
  const bool is_vertical_tab = tabs::utils::ShouldShowVerticalTabs(browser_);

  model_ = std::make_unique<BraveTabMenuModel>(
      this, browser_->GetFeatures().tab_menu_model_delegate(),
      controller->model(),
#if BUILDFLAG(ENABLE_CONTAINERS)
      *this,
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
      index, is_vertical_tab);
  menu_runner_ = std::make_unique<views::MenuRunner>(
      model_.get(),
      views::MenuRunner::HAS_MNEMONICS | views::MenuRunner::CONTEXT_MENU,
      base::BindRepeating(&BraveTabContextMenuContents::OnMenuClosed,
                          weak_ptr_.GetWeakPtr()));
}

BraveTabContextMenuContents::~BraveTabContextMenuContents() = default;

void BraveTabContextMenuContents::Cancel() {
  controller_ = nullptr;
}

void BraveTabContextMenuContents::RunMenuAt(
    const gfx::Point& point,
    ui::mojom::MenuSourceType source_type) {
  menu_runner_->RunMenuAt(tab_->GetWidget(), nullptr,
                          gfx::Rect(point, gfx::Size()),
                          views::MenuAnchorPosition::kTopLeft, source_type);
}

bool BraveTabContextMenuContents::IsCommandIdChecked(int command_id) const {
  return controller_->IsContextMenuCommandChecked(
      static_cast<TabStripModel::ContextMenuCommand>(command_id));
}

bool BraveTabContextMenuContents::IsCommandIdEnabled(int command_id) const {
  return controller_->IsCommandEnabledForTab(
      static_cast<TabStripModel::ContextMenuCommand>(command_id), tab_);
}

bool BraveTabContextMenuContents::GetAcceleratorForCommandId(
    int command_id,
    ui::Accelerator* accelerator) const {
  return controller_->GetContextMenuAccelerator(command_id, accelerator);
}

void BraveTabContextMenuContents::ExecuteCommand(int command_id,
                                                 int event_flags) {
  // Executing the command destroys |this|, and can also end up destroying
  // |controller_|. So stop the highlights before executing the command.
  controller_->ExecuteContextMenuCommand(
      tab_index_, static_cast<TabStripModel::ContextMenuCommand>(command_id),
      event_flags);
}

#if BUILDFLAG(ENABLE_CONTAINERS)
void BraveTabContextMenuContents::OnContainerSelected(
    const containers::mojom::ContainerPtr& container) {
  // Should open selected tabs in the specified container.

  // TODO(https://github.com/brave/brave-browser/issues/46352) Uncomment this
  // when the containers feature is ready.
  // auto* tab_strip_model =
  //     static_cast<BraveTabStripModel*>(controller_->model());
  // for (auto index : tab_strip_model->GetTabIndicesForCommandAt(tab_index_)) {
  //   auto* tab = tab_strip_model->GetTabAtIndex(index);
  //   brave::IsolateTab(browser_, tab->GetHandle(),
  //                     container->id);
  // }
  NOTIMPLEMENTED();
}

base::flat_set<std::string>
BraveTabContextMenuContents::GetCurrentContainerIds() {
  // TODO(https://github.com/brave/brave-browser/issues/46352) Fill the set with
  // container ids of tabs selected.
  NOTIMPLEMENTED();
  return {};
}

Browser* BraveTabContextMenuContents::GetBrowserToOpenSettings() {
  return browser_;
}

float BraveTabContextMenuContents::GetScaleFactor() {
  auto* widget = tab_->GetWidget();
  CHECK(widget);
  auto* compositor = widget->GetCompositor();
  CHECK(compositor);
  return compositor->device_scale_factor();
}
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

void BraveTabContextMenuContents::OnMenuClosed() {
  menu_closed_ = true;
  tab_ = nullptr;
}
