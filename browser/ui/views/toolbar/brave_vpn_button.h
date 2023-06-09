/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_BUTTON_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service_observer.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/button/menu_button_controller.h"

namespace brave_vpn {
class BraveVpnService;
}  // namespace brave_vpn

namespace views {
class Border;
}  // namespace views

class Browser;

class BraveVPNButton : public ToolbarButton,
                       public brave_vpn::BraveVPNServiceObserver {
 public:
  METADATA_HEADER(BraveVPNButton);

  explicit BraveVPNButton(Browser* browser);
  ~BraveVPNButton() override;

  BraveVPNButton(const BraveVPNButton&) = delete;
  BraveVPNButton& operator=(const BraveVPNButton&) = delete;

  // BraveVPNServiceObserver overrides:
  void OnConnectionStateChanged(
      brave_vpn::mojom::ConnectionState state) override;
  void OnPurchasedStateChanged(
      brave_vpn::mojom::PurchasedState state,
      const absl::optional<std::string>& description) override;

 private:
  // ToolbarButton overrides:
  void UpdateColorsAndInsets() override;
  std::u16string GetTooltipText(const gfx::Point& p) const override;

  bool IsConnected() const;
  bool IsConnectError() const;
  bool IsPurchased() const;
  std::unique_ptr<views::Border> GetBorder(SkColor border_color) const;
  void OnButtonPressed(const ui::Event& event);

  raw_ptr<Browser> browser_ = nullptr;
  raw_ptr<brave_vpn::BraveVpnService> service_ = nullptr;
  raw_ptr<views::MenuButtonController> menu_button_controller_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_BUTTON_H_
