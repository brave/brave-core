/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_BUTTON_H_

#include "base/scoped_observation.h"
#include "brave/browser/ui/webui/brave_vpn/vpn_panel_ui.h"
#include "brave/components/brave_vpn/brave_vpn_service_desktop.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveVPNButton : public ToolbarButton,
                       public BraveVpnServiceDesktop::Observer {
 public:
  METADATA_HEADER(BraveVPNButton);

  explicit BraveVPNButton(Profile* profile);
  ~BraveVPNButton() override;

  BraveVPNButton(const BraveVPNButton&) = delete;
  BraveVPNButton& operator=(const BraveVPNButton&) = delete;

  // BraveVpnService::Observer overrides:
  void OnConnectionStateChanged(bool connected) override;
  void OnConnectionCreated() override;
  void OnConnectionRemoved() override;

 private:
  void UpdateColorsAndInsets() override;

  // Update button based on connection state.
  void UpdateButtonState();
  bool IsConnected();

  void OnButtonPressed(const ui::Event& event);
  void ShowBraveVPNPanel();

  BraveVpnServiceDesktop* service_ = nullptr;
  base::ScopedObservation<BraveVpnServiceDesktop,
                          BraveVpnServiceDesktop::Observer>
      observation_{this};
  WebUIBubbleManagerT<VPNPanelUI> webui_bubble_manager_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_BUTTON_H_
