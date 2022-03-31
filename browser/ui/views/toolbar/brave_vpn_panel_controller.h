/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_PANEL_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_PANEL_CONTROLLER_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/webui/brave_vpn/vpn_panel_ui.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"

class BraveBrowserView;

class BraveVPNPanelController {
 public:
  explicit BraveVPNPanelController(BraveBrowserView* browser_view);
  ~BraveVPNPanelController();
  BraveVPNPanelController(const BraveVPNPanelController&) = delete;
  BraveVPNPanelController& operator=(const BraveVPNPanelController&) = delete;

  void ShowBraveVPNPanel();
  // Manager should be reset to use different anchor view for bubble.
  void ResetBubbleManager();

 private:
  raw_ptr<BraveBrowserView> browser_view_ = nullptr;
  std::unique_ptr<WebUIBubbleManagerT<VPNPanelUI>> webui_bubble_manager_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_PANEL_CONTROLLER_H_
