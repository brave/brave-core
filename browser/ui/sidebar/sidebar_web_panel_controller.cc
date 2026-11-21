/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_web_panel_controller.h"

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
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

void SidebarWebPanelController::ToggleWebPanel(const SidebarItem& item,
                                               bool suppress_animations) {
  const bool same_url = panel_contents_ && panel_item_.url == item.url;

  // Pressing the item again mid-close reverses the animation in place: the tab
  // is still there, so there is nothing to recreate.
  if (state_ == State::kClosing && same_url) {
    state_ = State::kOpen;
    GetMultiContentsView()->CancelWebPanelCloseAnimation();
    web_panel_state_changed_.Run();
    return;
  }

  if (state_ == State::kOpen && same_url) {
    CloseWebPanel(suppress_animations);
    return;
  }

  // Either a cold open, or one panel replacing another - including when the
  // item is the same but its url changed. Only a cold open that isn't taking
  // over the side panel's space animates; in every other case the strip is
  // already the width it will end up at.
  const bool had_panel = HasPanelTab();
  CloseWebPanelImmediately();
  OpenWebPanel(item, /*animate=*/!had_panel && !suppress_animations);
}

bool SidebarWebPanelController::HasOpenPanel() const {
  return HasPanelTab() && panel_item_.IsValidItem() && state_ == State::kOpen;
}

bool SidebarWebPanelController::HasPanelTab() const {
  return !!panel_contents_;
}

void SidebarWebPanelController::OpenWebPanel(const SidebarItem& item,
                                             bool animate) {
  panel_contents_ = chrome::AddAndReturnTabAt(
      browser_view_->browser(), item.url, 0, false, std::nullopt, true);
  panel_item_ = item;
  state_ = State::kOpen;
  GetMultiContentsView()->SetWebPanelContents(panel_contents_, animate);
  web_panel_state_changed_.Run();
}

void SidebarWebPanelController::CloseWebPanel(bool suppress_animations) {
  if (suppress_animations) {
    // Guarded on HasPanelTab(), so this also finishes a panel that is already
    // animating closed - it still occupies the space being taken over.
    CloseWebPanelImmediately();
    return;
  }

  if (!HasOpenPanel()) {
    return;
  }

  state_ = State::kClosing;
  if (GetMultiContentsView()->StartWebPanelCloseAnimation(
          base::BindOnce(&SidebarWebPanelController::FinishClose,
                         weak_factory_.GetWeakPtr()))) {
    // The panel is on its way out, so stop presenting it as active while it
    // animates.
    web_panel_state_changed_.Run();
    return;
  }

  FinishClose();
}

void SidebarWebPanelController::CloseWebPanelImmediately() {
  if (!HasPanelTab()) {
    return;
  }

  state_ = State::kClosing;
  GetMultiContentsView()->ResetWebPanelAnimation();
  FinishClose();
}

void SidebarWebPanelController::FinishClose() {
  // The tab may already be gone - closed from the tab strip, or taken by
  // shutdown, while the animation was still running.
  if (state_ != State::kClosing || !panel_contents_) {
    return;
  }

  chrome::CloseWebContents(browser_view_->browser(), panel_contents_, false);
}

void SidebarWebPanelController::CloseWebPanelForShutdown() {
  // Deliberately HasPanelTab(), not HasOpenPanel(): a panel animating closed
  // still has its tab in the strip, and leaving it there would both restore it
  // next launch and trip the destructor's CHECK.
  if (!HasPanelTab()) {
    return;
  }

  // chrome::CloseWebContents() hands the tab to the renderer whenever any frame
  // of the panel page has a beforeunload, unload, pagehide or visibilitychange
  // handler - most of the web - because TabStripModel::CloseWebContentses()
  // then skips the detach and waits for the reply. The tab would still be in
  // the strip while the caller snapshots the session on its next statement.
  // Detaching skips those handlers and removes the tab synchronously, which is
  // safe here because the shutdown paths have already run beforeunload for
  // every tab and the close can no longer be cancelled.
  auto* tab_strip_model = browser_view_->browser()->GetTabStripModel();
  const int index = tab_strip_model->GetIndexOfWebContents(panel_contents_);
  CHECK_NE(index, TabStripModel::kNoTab);
  tab_strip_model->DetachAndDeleteWebContentsAt(index);
}

void SidebarWebPanelController::ClearPanelState() {
  state_ = State::kClosed;
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

void SidebarWebPanelController::OnTabWillBeRemoved(tabs::TabHandle tab) {
  tabs::TabInterface* tab_interface = tab.Get();
  CHECK(tab_interface);
  if (panel_contents_ == tab_interface->GetContents()) {
    ClearPanelState();
  }
}

}  // namespace sidebar
