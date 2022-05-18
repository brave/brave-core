/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_STATUS_LABEL_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_STATUS_LABEL_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_vpn/brave_vpn_service_observer.h"
#include "ui/views/controls/label.h"

namespace brave_vpn {
class BraveVpnService;
}  // namespace brave_vpn

class Browser;

class BraveVPNStatusLabel : public views::Label,
                            public brave_vpn::BraveVPNServiceObserver {
 public:
  explicit BraveVPNStatusLabel(Browser* browser);
  ~BraveVPNStatusLabel() override;

  BraveVPNStatusLabel(const BraveVPNStatusLabel&) = delete;
  BraveVPNStatusLabel& operator=(const BraveVPNStatusLabel&) = delete;

  gfx::Size CalculatePreferredSize() const override;

 private:
  // brave_vpn::BraveVPNServiceObserver overrides:
  void OnConnectionStateChanged(
      brave_vpn::mojom::ConnectionState state) override;

  void UpdateState();

  int longest_state_string_id_ = -1;
  raw_ptr<Browser> browser_ = nullptr;
  raw_ptr<brave_vpn::BraveVpnService> service_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_STATUS_LABEL_H_
