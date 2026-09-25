/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_web_panel_controller.h"

#include <utility>

#include "base/types/to_address.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

namespace sidebar {

SidebarWebPanelController::SidebarWebPanelController(
    BrowserView& browser_view,
    base::RepeatingClosure web_panel_state_changed)
    : browser_view_(browser_view),
      web_panel_state_changed_(std::move(web_panel_state_changed)) {
  CHECK(IsWebPanelFeatureEnabled());
  browser_view_->browser()->GetTabStripModel()->AddObserver(this);
}

SidebarWebPanelController::~SidebarWebPanelController() {
  // When browser closes while panel is open, tab strip model will be cleared
  // and panel_contents_ is also will be cleared out from
  // SidebarWebPanelController::OnTabRemoved()
  CHECK(!panel_contents_);
}

void SidebarWebPanelController::ToggleWebPanel(const SidebarItem& item) {
  // If panel is for |item|, close and return.
  // Otherwise, open new panel after closing.
  // When item is same but url has been changed, closes the current panel and
  // reopens the panel with the new url.
  const bool close_and_return = IsShowingItem(item);
  ClosePanel();

  if (close_and_return) {
    return;
  }

  OpenWebPanel(item);

  // browser view could have different UI per web panel state.
  BraveBrowserView::From(base::to_address(browser_view_))
      ->UpdateRoundedCornersUI();
}

void SidebarWebPanelController::ClosePanel() {
  if (!HasOpenPanel()) {
    return;
  }

  chrome::CloseWebContents(browser_view_->browser(), panel_contents_, false);

  // Tab close can be deferred (ex, beforeunload), so clear now to keep
  // OpenWebPanel()'s precondition.
  CloseWebPanel();
}

void SidebarWebPanelController::OpenWebPanel(const SidebarItem& item) {
  CHECK(!panel_item_.IsValidItem());

  panel_contents_ = chrome::AddAndReturnTabAt(
      browser_view_->browser(), item.url, 0, false, std::nullopt, true);
  panel_item_ = item;
  GetMultiContentsView()->SetWebPanelContents(panel_contents_);
  web_panel_state_changed_.Run();
}

void SidebarWebPanelController::CloseWebPanel() {
  // Keep the state change notification single-shot as both ClosePanel() and
  // OnTabWillBeRemoved() can reach here for one close.
  if (!HasOpenPanel()) {
    return;
  }

  GetMultiContentsView()->SetWebPanelContents(nullptr);
  panel_contents_ = nullptr;
  panel_item_ = sidebar::SidebarItem();
  web_panel_state_changed_.Run();
}

BraveMultiContentsView* SidebarWebPanelController::GetMultiContentsView() {
  return const_cast<BraveMultiContentsView*>(
      std::as_const(*this).GetMultiContentsView());
}

const BraveMultiContentsView* SidebarWebPanelController::GetMultiContentsView()
    const {
  return static_cast<BraveMultiContentsView*>(
      browser_view_->multi_contents_view());
}

void SidebarWebPanelController::OnTabWillBeRemoved(tabs::TabInterface* tab,
                                                   int index) {
  if (panel_contents_ == tab->GetContents()) {
    CloseWebPanel();
  }
}

}  // namespace sidebar
