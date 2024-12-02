/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_tray_runner.h"

#include <memory>

#include "base/test/bind.h"
#include "base/test/test_reg_util_win.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/brave_vpn_tray_command_ids.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/resources/resource.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_icon/tray_menu_model.h"
#include "brave/browser/brave_vpn/win/storage_utils.h"
#include "components/grit/brave_components_strings.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/menus/simple_menu_model.h"
#include "ui/native_theme/native_theme.h"

namespace brave_vpn {

class StatusTrayRunnerTest : public testing::Test {
 public:
  void CheckConnectedMenuState(TrayMenuModel* menu_model) {
    EXPECT_EQ(menu_model->GetItemCount(), 7u);
    EXPECT_FALSE(menu_model->IsEnabledAt(0));
    EXPECT_TRUE(menu_model->IsVisibleAt(0));
    for (size_t index = 1; index < menu_model->GetItemCount(); index++) {
      EXPECT_TRUE(menu_model->IsEnabledAt(index));
      EXPECT_TRUE(menu_model->IsVisibleAt(index));
    }

    EXPECT_EQ(menu_model->GetCommandIdAt(1),
              IDC_BRAVE_VPN_TRAY_DISCONNECT_VPN_ITEM);
    EXPECT_FALSE(
        menu_model->GetIndexOfCommandId(IDC_BRAVE_VPN_TRAY_CONNECT_VPN_ITEM));
  }

  void CheckDisconnectedMenuState(TrayMenuModel* menu_model) {
    EXPECT_EQ(menu_model->GetItemCount(), 7u);
    EXPECT_FALSE(menu_model->IsEnabledAt(0));
    EXPECT_TRUE(menu_model->IsVisibleAt(0));
    for (size_t index = 1; index < menu_model->GetItemCount(); index++) {
      EXPECT_TRUE(menu_model->IsEnabledAt(index));
      EXPECT_TRUE(menu_model->IsVisibleAt(index));
    }

    EXPECT_EQ(menu_model->GetCommandIdAt(1),
              IDC_BRAVE_VPN_TRAY_CONNECT_VPN_ITEM);

    EXPECT_FALSE(menu_model->GetIndexOfCommandId(
        IDC_BRAVE_VPN_TRAY_DISCONNECT_VPN_ITEM));
  }

  void UpdateConnectionState() {
    StatusTrayRunner::GetInstance()->UpdateConnectionState();
  }

  void WaitIconStateChangedTo(int expected_icon_id, int expected_tooltip_id) {
    StatusTrayRunner::GetInstance()->SetIconStateCallbackForTesting(
        base::BindLambdaForTesting([&](int icon_id, int tooltip_id) {
          EXPECT_EQ(icon_id, expected_icon_id);
          EXPECT_EQ(tooltip_id, expected_tooltip_id);
          icon_state_updated_ = true;
        }));
    EXPECT_FALSE(IsIconStateUpdated());
    UpdateConnectionState();
    EXPECT_TRUE(IsIconStateUpdated());
    ResetIconState();
    EXPECT_FALSE(IsIconStateUpdated());
  }

  void ResetIconState() { icon_state_updated_ = false; }
  bool IsIconStateUpdated() const { return icon_state_updated_; }

 private:
  bool icon_state_updated_ = false;
};

TEST_F(StatusTrayRunnerTest, RebuildMenu) {
  TrayMenuModel menu_model(StatusTrayRunner::GetInstance());
  EXPECT_EQ(menu_model.GetItemCount(), 0u);

  // connected state
  StatusTrayRunner::GetInstance()->SetVPNConnectedForTesting(true);
  menu_model.MenuWillShow();
  CheckConnectedMenuState(&menu_model);

  // disconnected state
  StatusTrayRunner::GetInstance()->SetVPNConnectedForTesting(false);
  menu_model.MenuWillShow();
  CheckDisconnectedMenuState(&menu_model);

  // back to connected state
  StatusTrayRunner::GetInstance()->SetVPNConnectedForTesting(true);
  menu_model.MenuWillShow();
  CheckConnectedMenuState(&menu_model);
}

TEST_F(StatusTrayRunnerTest, UpdateConnectionState) {
  registry_util::RegistryOverrideManager registry_overrides;
  registry_overrides.OverrideRegistry(HKEY_CURRENT_USER);

  ui::NativeTheme::GetInstanceForNativeUi()->set_use_dark_colors(true);
  EXPECT_TRUE(ui::NativeTheme::GetInstanceForNativeUi()->ShouldUseDarkColors());

  // Tunnel service stopped, state disconnected, no info in registry.
  StatusTrayRunner::GetInstance()->SetVPNConnectedForTesting(false);
  WaitIconStateChangedTo(
      IDR_BRAVE_VPN_TRAY_LIGHT,
      IDS_BRAVE_VPN_WIREGUARD_TRAY_ICON_TOOLTIP_DISCONNECTED);
  EXPECT_FALSE(brave_vpn::GetConnectionState().has_value());

  // Tunnel service stopped, registry state as "connecting"
  WriteConnectionState(
      static_cast<int>(brave_vpn::mojom::ConnectionState::CONNECTING));
  WaitIconStateChangedTo(IDR_BRAVE_VPN_TRAY_LIGHT_CONNECTING,
                         IDS_BRAVE_VPN_WIREGUARD_TRAY_ICON_TOOLTIP_CONNECTING);
  EXPECT_EQ(brave_vpn::GetConnectionState().value(),
            static_cast<int>(brave_vpn::mojom::ConnectionState::CONNECTING));

  // Tunnel service stopped, registry state as "connected"
  WriteConnectionState(
      static_cast<int>(brave_vpn::mojom::ConnectionState::CONNECTED));
  EXPECT_EQ(brave_vpn::GetConnectionState().value(),
            static_cast<int>(brave_vpn::mojom::ConnectionState::CONNECTED));
  // State should not be shown as connected because tunnel service is not
  // launched.
  WaitIconStateChangedTo(
      IDR_BRAVE_VPN_TRAY_LIGHT,
      IDS_BRAVE_VPN_WIREGUARD_TRAY_ICON_TOOLTIP_DISCONNECTED);
  // Registry state was reset because the service wasn't running.
  EXPECT_EQ(brave_vpn::GetConnectionState().value(),
            static_cast<int>(brave_vpn::mojom::ConnectionState::DISCONNECTED));

  // Tunnel service stopped, registry state as "disconnecting"
  WriteConnectionState(
      static_cast<int>(brave_vpn::mojom::ConnectionState::DISCONNECTING));
  WaitIconStateChangedTo(
      IDR_BRAVE_VPN_TRAY_LIGHT,
      IDS_BRAVE_VPN_WIREGUARD_TRAY_ICON_TOOLTIP_DISCONNECTING);
  EXPECT_EQ(brave_vpn::GetConnectionState().value(),
            static_cast<int>(brave_vpn::mojom::ConnectionState::DISCONNECTING));

  // Tunnel service stopped, registry state as "CONNECT_NOT_ALLOWED"
  WriteConnectionState(
      static_cast<int>(brave_vpn::mojom::ConnectionState::CONNECT_NOT_ALLOWED));
  WaitIconStateChangedTo(IDR_BRAVE_VPN_TRAY_LIGHT_ERROR,
                         IDS_BRAVE_VPN_WIREGUARD_TRAY_ICON_TOOLTIP_ERROR);
  EXPECT_EQ(
      brave_vpn::GetConnectionState().value(),
      static_cast<int>(brave_vpn::mojom::ConnectionState::CONNECT_NOT_ALLOWED));

  // Tunnel service stopped, registry state as "CONNECT_FAILED"
  WriteConnectionState(
      static_cast<int>(brave_vpn::mojom::ConnectionState::CONNECT_FAILED));
  WaitIconStateChangedTo(IDR_BRAVE_VPN_TRAY_LIGHT_ERROR,
                         IDS_BRAVE_VPN_WIREGUARD_TRAY_ICON_TOOLTIP_ERROR);
  EXPECT_EQ(
      brave_vpn::GetConnectionState().value(),
      static_cast<int>(brave_vpn::mojom::ConnectionState::CONNECT_FAILED));

  // Service is working, state connected.
  StatusTrayRunner::GetInstance()->SetVPNConnectedForTesting(true);
  WaitIconStateChangedTo(IDR_BRAVE_VPN_TRAY_LIGHT_CONNECTED,
                         IDS_BRAVE_VPN_WIREGUARD_TRAY_ICON_TOOLTIP_CONNECTED);
  EXPECT_EQ(brave_vpn::GetConnectionState().value(),
            static_cast<int>(brave_vpn::mojom::ConnectionState::CONNECTED));
}

TEST_F(StatusTrayRunnerTest, SkipAttemptsToConnectInFailedState) {
  registry_util::RegistryOverrideManager registry_overrides;
  registry_overrides.OverrideRegistry(HKEY_CURRENT_USER);

  ui::NativeTheme::GetInstanceForNativeUi()->set_use_dark_colors(true);
  EXPECT_TRUE(ui::NativeTheme::GetInstanceForNativeUi()->ShouldUseDarkColors());
  // Tunnel service stopped, state disconnected, no info in registry.
  StatusTrayRunner::GetInstance()->SetVPNConnectedForTesting(false);
  WaitIconStateChangedTo(
      IDR_BRAVE_VPN_TRAY_LIGHT,
      IDS_BRAVE_VPN_WIREGUARD_TRAY_ICON_TOOLTIP_DISCONNECTED);
  EXPECT_FALSE(brave_vpn::GetConnectionState().has_value());

  // Tunnel service stopped, registry state as "connecting"
  WriteConnectionState(
      static_cast<int>(brave_vpn::mojom::ConnectionState::CONNECTING));
  WaitIconStateChangedTo(IDR_BRAVE_VPN_TRAY_LIGHT_CONNECTING,
                         IDS_BRAVE_VPN_WIREGUARD_TRAY_ICON_TOOLTIP_CONNECTING);
  EXPECT_EQ(brave_vpn::GetConnectionState().value(),
            static_cast<int>(brave_vpn::mojom::ConnectionState::CONNECTING));

  // Tunnel service stopped, registry state as "CONNECT_FAILED"
  WriteConnectionState(
      static_cast<int>(brave_vpn::mojom::ConnectionState::CONNECT_FAILED));
  WaitIconStateChangedTo(IDR_BRAVE_VPN_TRAY_LIGHT_ERROR,
                         IDS_BRAVE_VPN_WIREGUARD_TRAY_ICON_TOOLTIP_ERROR);
  EXPECT_EQ(
      brave_vpn::GetConnectionState().value(),
      static_cast<int>(brave_vpn::mojom::ConnectionState::CONNECT_FAILED));

  // Tunnel service stopped, registry state as "connecting"
  WriteConnectionState(
      static_cast<int>(brave_vpn::mojom::ConnectionState::CONNECTING));
  EXPECT_FALSE(IsIconStateUpdated());
  UpdateConnectionState();
  // Icon state should not be updated.
  EXPECT_FALSE(IsIconStateUpdated());

  // Tunnel service stopped, registry state as "disconnecting"
  WriteConnectionState(
      static_cast<int>(brave_vpn::mojom::ConnectionState::DISCONNECTING));
  EXPECT_FALSE(IsIconStateUpdated());
  UpdateConnectionState();
  // Icon state should not be updated.
  EXPECT_FALSE(IsIconStateUpdated());

  // Service is working, state connected.
  StatusTrayRunner::GetInstance()->SetVPNConnectedForTesting(true);
  WaitIconStateChangedTo(IDR_BRAVE_VPN_TRAY_LIGHT_CONNECTED,
                         IDS_BRAVE_VPN_WIREGUARD_TRAY_ICON_TOOLTIP_CONNECTED);
  EXPECT_EQ(brave_vpn::GetConnectionState().value(),
            static_cast<int>(brave_vpn::mojom::ConnectionState::CONNECTED));
}
}  // namespace brave_vpn
