/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_web_panel_controller.h"

#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

namespace sidebar {

SidebarWebPanelController::SidebarWebPanelController(BrowserView& browser_view)
    : browser_view_(browser_view) {
  CHECK(IsWebPanelFeatureEnabled());
  browser_view_->browser()->tab_strip_model()->AddObserver(this);
}

SidebarWebPanelController::~SidebarWebPanelController() {
  CHECK(!panel_contents_);
}

void SidebarWebPanelController::ToggleWebPanel(const SidebarItem& item) {
  // If panel is for |item|, close and return.
  // Otherwise, open new panel after closing.
  const bool close_and_return =
      panel_item_.IsValidItem() && panel_item_.url == item.url;
  if (panel_item_.IsValidItem()) {
    chrome::CloseWebContents(browser_view_->browser(), panel_contents_, false);
  }

  if (close_and_return) {
    return;
  }

  OpenWebPanel(item);
}

void SidebarWebPanelController::OpenWebPanel(const SidebarItem& item) {
  panel_contents_ = chrome::AddAndReturnTabAt(
      browser_view_->browser(), item.url, 0, false, std::nullopt, true);
  panel_item_ = item;
  GetMultiContentsView()->SetWebPanelContents(panel_contents_);
  GetMultiContentsView()->SetWebPanelVisible(true);
}

void SidebarWebPanelController::CloseWebPanel() {
  GetMultiContentsView()->SetWebPanelVisible(false);
  GetMultiContentsView()->SetWebPanelContents(nullptr);
  panel_contents_ = nullptr;
  panel_item_ = sidebar::SidebarItem();
}

bool SidebarWebPanelController::IsShowingWebPanel() const {
  return GetMultiContentsView()->IsWebPanelVisible();
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

void SidebarWebPanelController::OnTabWillBeRemoved(
    content::WebContents* contents,
    int index) {
  if (panel_contents_ == contents) {
    CloseWebPanel();
  }
}

}  // namespace sidebar
