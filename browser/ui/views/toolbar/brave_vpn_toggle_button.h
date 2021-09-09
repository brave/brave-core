/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_TOGGLE_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_TOGGLE_BUTTON_H_

#include "base/scoped_observation.h"
#include "brave/components/brave_vpn/brave_vpn_service_desktop.h"
#include "ui/views/controls/button/toggle_button.h"

class Browser;

class BraveVPNToggleButton : public views::ToggleButton,
                             public BraveVpnServiceDesktop::Observer {
 public:
  explicit BraveVPNToggleButton(Browser* browser);
  ~BraveVPNToggleButton() override;

  BraveVPNToggleButton(const BraveVPNToggleButton&) = delete;
  BraveVPNToggleButton& operator=(const BraveVPNToggleButton&) = delete;

 private:
  // BraveVpnServiceDesktop::Observer overrides:
  void OnConnectionStateChanged(ConnectionState state) override;

  void OnButtonPressed(const ui::Event& event);
  void UpdateState();
  void UpdateColors();

  Browser* browser_ = nullptr;
  BraveVpnServiceDesktop* service_ = nullptr;
  base::ScopedObservation<BraveVpnServiceDesktop,
                          BraveVpnServiceDesktop::Observer>
      observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_TOGGLE_BUTTON_H_
