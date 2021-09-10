/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_STATUS_LABEL_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_STATUS_LABEL_H_

#include "base/scoped_observation.h"
#include "brave/components/brave_vpn/brave_vpn_service_desktop.h"
#include "ui/views/controls/label.h"

class Browser;

class BraveVPNStatusLabel : public views::Label,
                            public BraveVpnServiceDesktop::Observer {
 public:
  explicit BraveVPNStatusLabel(Browser* browser);
  ~BraveVPNStatusLabel() override;

  BraveVPNStatusLabel(const BraveVPNStatusLabel&) = delete;
  BraveVPNStatusLabel& operator=(const BraveVPNStatusLabel&) = delete;

 private:
  // BraveVpnServiceDesktop::Observer overrides:
  void OnConnectionStateChanged(ConnectionState state) override;

  void UpdateState();

  Browser* browser_ = nullptr;
  BraveVpnServiceDesktop* service_ = nullptr;
  base::ScopedObservation<BraveVpnServiceDesktop,
                          BraveVpnServiceDesktop::Observer>
      observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_STATUS_LABEL_H_
