/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_tray_runner.h"

#include <memory>

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/brave_vpn_tray_command_ids.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_icon/tray_menu_model.h"
#include "brave/components/constants/brave_paths.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/models/simple_menu_model.h"

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
};

TEST_F(StatusTrayRunnerTest, RebuildMenu) {
  TrayMenuModel menu_model(StatusTrayRunner::GetInstance());
  EXPECT_EQ(menu_model.GetItemCount(), 0u);

  // connected state
  StatusTrayRunner::GetInstance()->SetTunnelServiceRunningForTesting(true);
  menu_model.MenuWillShow();
  CheckConnectedMenuState(&menu_model);

  // disconnected state
  StatusTrayRunner::GetInstance()->SetTunnelServiceRunningForTesting(false);
  menu_model.MenuWillShow();
  CheckDisconnectedMenuState(&menu_model);

  // back to connected state
  StatusTrayRunner::GetInstance()->SetTunnelServiceRunningForTesting(true);
  menu_model.MenuWillShow();
  CheckConnectedMenuState(&menu_model);
}

TEST_F(StatusTrayRunnerTest, FindPakPath) {
  base::FilePath test_data_dir;
  base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
  base::FilePath wireguard = test_data_dir.Append(L"wireguard");
  // Looking to upper directory.
  EXPECT_EQ(StatusTrayRunner::GetInstance()->FindPakFilePath(
                wireguard.Append(L"BraveVpnWireguardService"), "en-US"),
            base::FilePath(wireguard.Append(L"Locales").Append(L"en-US.pak")));
  // Looking to current directory.
  EXPECT_EQ(
      StatusTrayRunner::GetInstance()->FindPakFilePath(wireguard, "en-US"),
      base::FilePath(wireguard.Append(L"Locales").Append(L"en-US.pak")));
  // Fallback to english locale.
  EXPECT_EQ(
      StatusTrayRunner::GetInstance()->FindPakFilePath(wireguard, "de-DE"),
      base::FilePath(wireguard.Append(L"Locales").Append(L"en-US.pak")));
}

}  // namespace brave_vpn
