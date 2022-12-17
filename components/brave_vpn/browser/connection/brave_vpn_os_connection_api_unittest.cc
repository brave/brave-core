/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/brave_vpn_os_connection_api.h"

#include <memory>
#include <utility>

#include "base/run_loop.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_os_connection_api_sim.h"
#include "brave/components/brave_vpn/common/brave_vpn_data_types.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "build/build_config.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn {

namespace {
const char kProfileCredentialData[] = R"(
        {
          "eap-username": "brave-user",
          "eap-password": "brave-pwd"
        }
      )";
const char kHostNamesTestData[] = R"([
        {
          "hostname": "host-1.brave.com",
          "display-name": "host-1",
          "offline": false,
          "capacity-score": 0
        },
        {
          "hostname": "host-2.brave.com",
          "display-name": "host-2",
          "offline": false,
          "capacity-score": 1
        },
        {
          "hostname": "host-3.brave.com",
          "display-name": "Singapore",
          "offline": false,
          "capacity-score": 0
        },
        {
          "hostname": "host-4.brave.com",
          "display-name": "host-4",
          "offline": false,
          "capacity-score": 0
        },
        {
          "hostname": "host-5.brave.com",
          "display-name": "host-5",
          "offline": false,
          "capacity-score": 1
        }
      ])";
}  // namespace

class BraveVPNOSConnectionAPIUnitTest : public testing::Test {
 public:
  BraveVPNOSConnectionAPIUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    brave_vpn::RegisterLocalStatePrefs(local_pref_service_.registry());
    connection_api_ = BraveVPNOSConnectionAPI::GetInstanceForTest();
    connection_api_->SetLocalPrefs(local_state());
  }
  PrefService* local_state() { return &local_pref_service_; }
  BraveVPNOSConnectionAPI* GetConnectionAPI() { return connection_api_.get(); }

 private:
  TestingPrefServiceSimple local_pref_service_;
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<BraveVPNOSConnectionAPI> connection_api_;
};

// Create os vpn entry with cached connection_info when there is cached
// connection info.
TEST_F(BraveVPNOSConnectionAPIUnitTest,
       CreateOSVPNEntryWithValidInfoWhenConnectTest) {
  GetConnectionAPI()->CheckConnection();

  // Prepare valid connection info.
  auto* test_api = static_cast<BraveVPNOSConnectionAPISim*>(GetConnectionAPI());
  test_api->OnFetchHostnames("region-a", kHostNamesTestData, true);
  test_api->SetPreventCreationForTesting(true);
  test_api->OnGetProfileCredentials(kProfileCredentialData, true);
  EXPECT_TRUE(test_api->connection_info().IsValid());
  test_api->Connect();
  base::RunLoop().RunUntilIdle();
  // With cached connection info, connect process starts with
  // os vpn entry creation.

  EXPECT_TRUE(test_api->IsConnectionCreated());
}

TEST_F(BraveVPNOSConnectionAPIUnitTest, CreateOSVPNEntryWithInvalidInfoTest) {
  GetConnectionAPI()->CheckConnection();
  local_state()->SetString(prefs::kBraveVPNSelectedRegion, "region-a");
  // Prepare valid connection info.
  auto* test_api = static_cast<BraveVPNOSConnectionAPISim*>(GetConnectionAPI());
  test_api->OnFetchHostnames("region-a", kHostNamesTestData, true);
  test_api->SetPreventCreationForTesting(true);
  test_api->OnGetProfileCredentials(kProfileCredentialData, true);
  test_api->ResetConnectionInfo();
  // W/o valid connection info, connect will not try to create
  // os vpn entry at the beginning.
  EXPECT_FALSE(test_api->connection_info().IsValid());
  GetConnectionAPI()->Connect();
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(test_api->IsConnectionCreated());
}

TEST_F(BraveVPNOSConnectionAPIUnitTest, NeedsConnectTest) {
  auto* test_api =
      static_cast<BraveVPNOSConnectionAPIBase*>(GetConnectionAPI());

  GetConnectionAPI()->CheckConnection();

  // Check ignore Connect() request while connecting or disconnecting is
  // in-progress.
  local_state()->SetString(prefs::kBraveVPNSelectedRegion, "eu-es");
  test_api->connection_state_ = mojom::ConnectionState::CONNECTING;
  test_api->Connect();
  EXPECT_EQ(mojom::ConnectionState::CONNECTING, test_api->GetConnectionState());

  test_api->connection_state_ = mojom::ConnectionState::DISCONNECTING;
  test_api->Connect();
  EXPECT_EQ(mojom::ConnectionState::DISCONNECTING,
            test_api->GetConnectionState());

  // Handle connect after disconnect current connection.
  test_api->connection_state_ = mojom::ConnectionState::CONNECTED;
  test_api->Connect();
  EXPECT_TRUE(test_api->needs_connect_);
  EXPECT_EQ(mojom::ConnectionState::DISCONNECTING,
            test_api->GetConnectionState());
  test_api->OnDisconnected();
  EXPECT_FALSE(test_api->needs_connect_);
  EXPECT_EQ(mojom::ConnectionState::CONNECTING, test_api->GetConnectionState());
}

TEST_F(BraveVPNOSConnectionAPIUnitTest,
       CheckConnectionStateAfterNetworkStateChanged) {
  auto* test_api = static_cast<BraveVPNOSConnectionAPISim*>(GetConnectionAPI());
  EXPECT_FALSE(test_api->IsConnectionChecked());
  test_api->OnNetworkChanged(net::NetworkChangeNotifier::CONNECTION_WIFI);
  EXPECT_TRUE(test_api->IsConnectionChecked());
}

TEST_F(BraveVPNOSConnectionAPIUnitTest, HostnamesTest) {
  auto* test_api =
      static_cast<BraveVPNOSConnectionAPIBase*>(GetConnectionAPI());
  // Set valid hostnames list
  test_api->hostname_.reset();
  test_api->OnFetchHostnames("region-a", kHostNamesTestData, true);
  // Check best one is picked from fetched hostname list.
  EXPECT_EQ("host-2.brave.com", test_api->hostname_->hostname);

  // Can't get hostname from invalid hostnames list
  test_api->hostname_.reset();
  test_api->OnFetchHostnames("invalid-region-b", "", false);
  EXPECT_FALSE(test_api->hostname_);
}

TEST_F(BraveVPNOSConnectionAPIUnitTest, ConnectionInfoTest) {
  auto* test_api =
      static_cast<BraveVPNOSConnectionAPIBase*>(GetConnectionAPI());

  // Check valid connection info is set when valid hostname and profile
  // credential are fetched.
  test_api->connection_state_ = mojom::ConnectionState::CONNECTING;
  test_api->OnFetchHostnames("region-a", kHostNamesTestData, true);
  EXPECT_EQ(mojom::ConnectionState::CONNECTING, test_api->GetConnectionState());

  // To prevent real os vpn entry creation.
  test_api->prevent_creation_ = true;
  test_api->OnGetProfileCredentials(kProfileCredentialData, true);
  EXPECT_EQ(mojom::ConnectionState::CONNECTING, test_api->GetConnectionState());
  EXPECT_TRUE(test_api->connection_info().IsValid());

  // Check cached connection info is cleared when user set new selected region.
  test_api->connection_state_ = mojom::ConnectionState::DISCONNECTED;
  GetConnectionAPI()->ResetConnectionInfo();
  EXPECT_FALSE(test_api->connection_info().IsValid());

  // Fill connection info again.
  test_api->OnGetProfileCredentials(kProfileCredentialData, true);
  EXPECT_TRUE(test_api->connection_info().IsValid());

  // Check cached connection info is cleared when connect failed.
  test_api->OnConnectFailed();
  EXPECT_FALSE(test_api->connection_info().IsValid());
}

TEST_F(BraveVPNOSConnectionAPIUnitTest, CancelConnectingTest) {
  auto* test_api =
      static_cast<BraveVPNOSConnectionAPIBase*>(GetConnectionAPI());

  GetConnectionAPI()->CheckConnection();

  test_api->cancel_connecting_ = true;
  test_api->connection_state_ = mojom::ConnectionState::CONNECTING;
  test_api->OnCreated();
  EXPECT_FALSE(test_api->cancel_connecting_);
  EXPECT_EQ(mojom::ConnectionState::DISCONNECTED,
            test_api->GetConnectionState());

  // Start disconnect() when connect is done for cancelling.
  test_api->cancel_connecting_ = false;
  test_api->connection_state_ = mojom::ConnectionState::CONNECTING;
  test_api->Disconnect();
  EXPECT_TRUE(test_api->cancel_connecting_);
  EXPECT_EQ(mojom::ConnectionState::DISCONNECTING,
            test_api->GetConnectionState());
  test_api->OnConnected();
  EXPECT_FALSE(test_api->cancel_connecting_);
  EXPECT_EQ(mojom::ConnectionState::DISCONNECTING,
            test_api->GetConnectionState());

  test_api->cancel_connecting_ = false;
  test_api->connection_state_ = mojom::ConnectionState::CONNECTING;
  test_api->Disconnect();
  EXPECT_TRUE(test_api->cancel_connecting_);
  EXPECT_EQ(mojom::ConnectionState::DISCONNECTING,
            test_api->GetConnectionState());

  test_api->cancel_connecting_ = true;
  test_api->CreateVPNConnection();
  EXPECT_FALSE(test_api->cancel_connecting_);
  EXPECT_EQ(mojom::ConnectionState::DISCONNECTED, test_api->connection_state_);

  test_api->cancel_connecting_ = true;
  test_api->connection_state_ = mojom::ConnectionState::CONNECTING;
  test_api->OnFetchHostnames("", "", true);
  EXPECT_FALSE(test_api->cancel_connecting_);
  EXPECT_EQ(mojom::ConnectionState::DISCONNECTED,
            test_api->GetConnectionState());

  test_api->cancel_connecting_ = true;
  test_api->connection_state_ = mojom::ConnectionState::CONNECTING;
  test_api->OnGetProfileCredentials("", true);
  EXPECT_FALSE(test_api->cancel_connecting_);
  EXPECT_EQ(mojom::ConnectionState::DISCONNECTED,
            test_api->GetConnectionState());
}

// Ignore disconnected state change while connected. See the comment at
// BraveVPNOSConnectionAPI::UpdateAndNotifyConnectionStateChange().
TEST_F(BraveVPNOSConnectionAPIUnitTest,
       IgnoreDisconnectedStateWhileConnecting) {
  auto* test_api =
      static_cast<BraveVPNOSConnectionAPIBase*>(GetConnectionAPI());

  test_api->SetConnectionState(mojom::ConnectionState::CONNECTING);
  test_api->UpdateAndNotifyConnectionStateChange(
      mojom::ConnectionState::DISCONNECTED);
  EXPECT_EQ(mojom::ConnectionState::CONNECTING, test_api->GetConnectionState());
}

}  // namespace brave_vpn
