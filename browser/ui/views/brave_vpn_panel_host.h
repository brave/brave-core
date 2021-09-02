/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_VPN_PANEL_HOST_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_VPN_PANEL_HOST_H_

#include <memory>

#include "brave/browser/ui/webui/brave_vpn/vpn_panel_ui.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"

class BraveBrowserView;

class BraveVPNPanelHost {
 public:
  explicit BraveVPNPanelHost(BraveBrowserView* browser_view);
  ~BraveVPNPanelHost();
  BraveVPNPanelHost(const BraveVPNPanelHost&) = delete;
  BraveVPNPanelHost& operator=(const BraveVPNPanelHost&) = delete;

  void ShowBraveVPNPanel();

 private:
  BraveBrowserView* browser_view_;
  std::unique_ptr<WebUIBubbleManagerT<VPNPanelUI>> webui_bubble_manager_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_VPN_PANEL_HOST_H_
