/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_web_panel_controller.h"

#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

namespace sidebar {

SidebarWebPanelController::SidebarWebPanelController(BrowserView& browser_view)
    : browser_view_(browser_view) {
  CHECK(IsWebPanelFeatureEnabled());
}

SidebarWebPanelController::~SidebarWebPanelController() = default;

void SidebarWebPanelController::OpenWebPanel(const SidebarItem& item) {
  GetMultiContentsView()->SetWebPanelVisible(true);
}

void SidebarWebPanelController::CloseWebPanel() {
  GetMultiContentsView()->SetWebPanelVisible(false);
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

}  // namespace sidebar
