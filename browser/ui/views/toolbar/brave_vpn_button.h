/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_BUTTON_H_

#include "brave/components/brave_vpn/brave_vpn_service_observer.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveVpnServiceDesktop;
class Browser;

class BraveVPNButton : public ToolbarButton, public BraveVPNServiceObserver {
 public:
  METADATA_HEADER(BraveVPNButton);

  explicit BraveVPNButton(Browser* browser);
  ~BraveVPNButton() override;

  BraveVPNButton(const BraveVPNButton&) = delete;
  BraveVPNButton& operator=(const BraveVPNButton&) = delete;

  // BraveVPNServiceObserver overrides:
  void OnConnectionStateChanged(
      brave_vpn::mojom::ConnectionState state) override;

 private:
  void UpdateColorsAndInsets() override;

  // Update button based on connection state.
  void UpdateButtonState();
  bool IsConnected();

  void OnButtonPressed(const ui::Event& event);

  Browser* browser_ = nullptr;
  BraveVpnServiceDesktop* service_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_BUTTON_H_
