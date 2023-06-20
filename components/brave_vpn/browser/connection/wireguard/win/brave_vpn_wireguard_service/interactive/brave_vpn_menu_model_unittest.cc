/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/interactive/status_tray/brave_vpn_menu_model.h"

#include <memory>

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/interactive/status_tray/brave_vpn_tray_command_ids.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn {

class BraveVpnMenuModelTest : public testing::Test {
 public:
  void SetUp() override {
    menu_ = std::make_unique<BraveVpnMenuModel>(nullptr);
  }

  BraveVpnMenuModel* menu_model() { return menu_.get(); }

  void CheckConnectedMenuState() {
    EXPECT_EQ(menu_model()->GetItemCount(), 7u);
    EXPECT_FALSE(menu_model()->IsEnabledAt(0));
    EXPECT_TRUE(menu_model()->IsVisibleAt(0));
    for (size_t index = 1; index < menu_model()->GetItemCount(); index++) {
      EXPECT_TRUE(menu_model()->IsCommandIdEnabled(index));
      EXPECT_TRUE(menu_model()->IsVisibleAt(index));
    }

    EXPECT_EQ(menu_model()->GetCommandIdAt(1),
              IDC_BRAVE_VPN_TRAY_DISCONNECT_VPN_ITEM);
    EXPECT_FALSE(
        menu_model()->GetIndexOfCommandId(IDC_BRAVE_VPN_TRAY_CONNECT_VPN_ITEM));
  }

  void CheckDisconnectedMenuState() {
    EXPECT_EQ(menu_model()->GetItemCount(), 7u);
    EXPECT_FALSE(menu_model()->IsEnabledAt(0));
    EXPECT_TRUE(menu_model()->IsVisibleAt(0));
    for (size_t index = 1; index < menu_model()->GetItemCount(); index++) {
      EXPECT_TRUE(menu_model()->IsCommandIdEnabled(index));
      EXPECT_TRUE(menu_model()->IsVisibleAt(index));
    }

    EXPECT_EQ(menu_model()->GetCommandIdAt(1),
              IDC_BRAVE_VPN_TRAY_CONNECT_VPN_ITEM);

    EXPECT_FALSE(menu_model()->GetIndexOfCommandId(
        IDC_BRAVE_VPN_TRAY_DISCONNECT_VPN_ITEM));
  }

 private:
  std::unique_ptr<BraveVpnMenuModel> menu_;
};

TEST_F(BraveVpnMenuModelTest, Rebuild) {
  // connected state
  menu_model()->RebuildMenu(true);
  CheckConnectedMenuState();
  // disconnected state
  menu_model()->RebuildMenu(false);
  CheckDisconnectedMenuState();
  // back to connected state
  menu_model()->RebuildMenu(true);
  CheckConnectedMenuState();
}

}  // namespace brave_vpn
