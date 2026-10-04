/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_VPN_BRAVE_VPN_CONTROLLER_H_
#define BRAVE_BROWSER_UI_BRAVE_VPN_BRAVE_VPN_CONTROLLER_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

class BraveBrowserView;
class BrowserView;
class BrowserWindowInterface;

class BraveVPNController {
 public:
  DECLARE_USER_DATA(BraveVPNController);

  explicit BraveVPNController(BrowserView* browser_view);
  ~BraveVPNController();
  BraveVPNController(const BraveVPNController&) = delete;
  BraveVPNController& operator=(const BraveVPNController&) = delete;

  // Returns the instance owned by `browser`, or nullptr.
  static BraveVPNController* From(BrowserWindowInterface* browser);

  void ShowBraveVPNBubble(bool show_select = false);
  void OpenVPNAccountPage(brave_vpn::mojom::ManageURLType type);

 private:
  BraveBrowserView* GetBraveBrowserView();

  raw_ptr<BrowserView> browser_view_ = nullptr;
  ui::ScopedUnownedUserData<BraveVPNController> scoped_unowned_user_data_;
};

#endif  // BRAVE_BROWSER_UI_BRAVE_VPN_BRAVE_VPN_CONTROLLER_H_
