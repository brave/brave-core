/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_VPN_BRAVE_VPN_CONTROLLER_H_
#define BRAVE_BROWSER_UI_BRAVE_VPN_BRAVE_VPN_CONTROLLER_H_

#include <memory>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "components/prefs/pref_change_registrar.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

class BrowserWindowInterface;
class WebUIBubbleManager;

namespace views {
class View;
}  // namespace views

class BraveVPNController {
 public:
  DECLARE_USER_DATA(BraveVPNController);

  explicit BraveVPNController(BrowserWindowInterface& browser);
  ~BraveVPNController();
  BraveVPNController(const BraveVPNController&) = delete;
  BraveVPNController& operator=(const BraveVPNController&) = delete;

  // Returns the instance owned by `browser`, or nullptr.
  static BraveVPNController* From(BrowserWindowInterface* browser);

  // Toggles the VPN panel. Pass true to `show_select` to open it on the server
  // selection.
  void ShowBraveVPNBubble(bool show_select = false);
  void OpenVPNAccountPage(brave_vpn::mojom::ManageURLType type);

  views::View* GetAnchorViewForTesting() { return GetAnchorView(); }

 private:
  // The VPN toolbar button while it is shown, otherwise the app menu button.
  views::View* GetAnchorView();

  // The bubble manager keeps the anchor it was shown from, so it is recreated
  // when the anchor changes.
  void ResetBubbleManager();

  const raw_ref<BrowserWindowInterface> browser_;
  PrefChangeRegistrar pref_change_registrar_;
  std::unique_ptr<WebUIBubbleManager> webui_bubble_manager_;
  ui::ScopedUnownedUserData<BraveVPNController> scoped_unowned_user_data_;
};

#endif  // BRAVE_BROWSER_UI_BRAVE_VPN_BRAVE_VPN_CONTROLLER_H_
