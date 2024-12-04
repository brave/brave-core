/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_BUTTON_H_

#include <memory>
#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service_observer.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/button/menu_button_controller.h"

namespace brave_vpn {
class BraveVpnService;
class BraveVpnButtonUnitTest;
}  // namespace brave_vpn

namespace views {
class Border;
}  // namespace views

class Browser;

class BraveVPNButton : public ToolbarButton,
                       public brave_vpn::BraveVPNServiceObserver {
  METADATA_HEADER(BraveVPNButton, ToolbarButton)
 public:

  explicit BraveVPNButton(Browser* browser);
  ~BraveVPNButton() override;

  BraveVPNButton(const BraveVPNButton&) = delete;
  BraveVPNButton& operator=(const BraveVPNButton&) = delete;

  // BraveVPNServiceObserver overrides:
  void OnConnectionStateChanged(
      brave_vpn::mojom::ConnectionState state) override;
  void OnPurchasedStateChanged(
      brave_vpn::mojom::PurchasedState state,
      const std::optional<std::string>& description) override;

 private:
  friend class brave_vpn::BraveVpnButtonUnitTest;

  // ToolbarButton overrides:
  void UpdateColorsAndInsets() override;
  std::u16string GetTooltipText(const gfx::Point& p) const override;
  void OnThemeChanged() override;
  void InkDropRippleAnimationEnded(views::InkDropState state) override;

  void SetVpnConnectionStateForTesting(
      brave_vpn::mojom::ConnectionState state) {
    connection_state_for_testing_ = state;
  }
  brave_vpn::mojom::ConnectionState GetVpnConnectionState() const;
  bool IsErrorState() const { return is_error_state_; }
  bool IsConnected() const;
  bool IsConnectError() const;
  bool IsPurchased() const;
  std::unique_ptr<views::Border> GetBorder(SkColor border_color) const;
  void OnButtonPressed(const ui::Event& event);
  void UpdateButtonState();
  SkColor GetIconColor();
  SkColor GetBadgeColor();
  const gfx::VectorIcon& GetBadgeIcon();

  bool is_error_state_ = false;
  bool is_connected_ = false;
  std::optional<brave_vpn::mojom::ConnectionState>
      connection_state_for_testing_;
  raw_ptr<Browser, DanglingUntriaged> browser_ = nullptr;
  raw_ptr<brave_vpn::BraveVpnService, DanglingUntriaged> service_ = nullptr;
  raw_ptr<views::MenuButtonController> menu_button_controller_ = nullptr;
  base::WeakPtrFactory<BraveVPNButton> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_VPN_BUTTON_H_
