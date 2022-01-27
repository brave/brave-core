/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/brave_vpn_panel_controller.h"

#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "url/gurl.h"

BraveVPNPanelController::BraveVPNPanelController(BraveBrowserView* browser_view)
    : browser_view_(browser_view) {
  DCHECK(browser_view_);
}

BraveVPNPanelController::~BraveVPNPanelController() = default;

void BraveVPNPanelController::ShowBraveVPNPanel() {
  auto* anchor_view = browser_view_->GetAnchorViewForBraveVPNPanel();
  if (!anchor_view)
    return;

  if (!webui_bubble_manager_) {
    auto* profile = browser_view_->browser()->profile();
    webui_bubble_manager_ = std::make_unique<WebUIBubbleManagerT<VPNPanelUI>>(
        anchor_view, profile, GURL(kVPNPanelURL), 1);
  }

  if (webui_bubble_manager_->GetBubbleWidget()) {
    webui_bubble_manager_->CloseBubble();
    return;
  }

  webui_bubble_manager_->ShowBubble();
}

void BraveVPNPanelController::ResetBubbleManager() {
  webui_bubble_manager_.reset();
}
