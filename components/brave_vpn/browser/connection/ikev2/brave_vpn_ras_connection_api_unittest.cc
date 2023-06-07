/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/brave_vpn_os_connection_api.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service_helper.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_region_data_manager.h"
#include "brave/components/brave_vpn/browser/connection/ikev2/brave_vpn_ras_connection_api_sim.h"
#include "brave/components/brave_vpn/common/brave_vpn_data_types.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
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
    connection_api_ = std::make_unique<BraveVPNOSConnectionAPISim>(
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_),
        local_state());
  }

  void OnFetchRegionList(const std::string& region_list, bool success) {
    GetBraveVPNConnectionAPIBase()->GetRegionDataManager().OnFetchRegionList(
        region_list, success);
  }

  void OnFetchTimezones(const std::string& timezones_list, bool success) {
    GetBraveVPNConnectionAPIBase()->GetRegionDataManager().OnFetchTimezones(
        timezones_list, success);
  }

  std::string GetTimeZonesData() {
    return R"([
        {
          "name": "us-central",
          "timezones": [
            "America/Guatemala",
            "America/Guayaquil",
            "America/Guyana",
            "America/Havana"
          ]
        },
        {
          "name": "eu-es",
          "timezones": [
            "Europe/Madrid",
            "Europe/Gibraltar",
            "Africa/Casablanca",
            "Africa/Algiers"
          ]
        },
        {
          "name": "eu-ch",
          "timezones": [
            "Europe/Zurich"
          ]
        },
        {
          "name": "eu-nl",
          "timezones": [
            "Europe/Amsterdam",
            "Europe/Brussels"
          ]
        },
        {
          "name": "asia-sg",
          "timezones": [
            "Asia/Aden",
            "Asia/Almaty",
            "Asia/Seoul"
          ]
        },
        {
          "name": "asia-jp",
          "timezones": [
            "Pacific/Guam",
            "Pacific/Saipan",
            "Asia/Tokyo"
          ]
        }
      ])";
  }

  std::string GetRegionsData() {
    // Give 11 region data.
    return R"([
        {
          "continent": "europe",
          "name": "eu-es",
          "name-pretty": "Spain"
        },
        {
          "continent": "south-america",
          "name": "sa-br",
          "name-pretty": "Brazil"
        },
        {
          "continent": "europe",
          "name": "eu-ch",
          "name-pretty": "Switzerland"
        },
        {
          "continent": "europe",
          "name": "eu-de",
          "name-pretty": "Germany"
        },
        {
          "continent": "asia",
          "name": "asia-sg",
          "name-pretty": "Singapore"
        },
        {
          "continent": "north-america",
          "name": "ca-east",
          "name-pretty": "Canada"
        },
        {
          "continent": "asia",
          "name": "asia-jp",
          "name-pretty": "Japan"
        },
        {
          "continent": "europe",
          "name": "eu-en",
          "name-pretty": "United Kingdom"
        },
        {
          "continent": "europe",
          "name": "eu-nl",
          "name-pretty": "Netherlands"
        },
        {
          "continent": "north-america",
          "name": "us-west",
          "name-pretty": "USA West"
        },
        {
          "continent": "oceania",
          "name": "au-au",
          "name-pretty": "Australia"
        }
      ])";
  }

  BraveVPNOSConnectionAPIBase* GetBraveVPNConnectionAPIBase() const {
    return static_cast<BraveVPNOSConnectionAPIBase*>(connection_api_.get());
  }

  void SetFallbackDeviceRegion() {
    GetBraveVPNConnectionAPIBase()
        ->GetRegionDataManager()
        .SetFallbackDeviceRegion();
  }

  void SetTestTimezone(const std::string& timezone) {
    GetBraveVPNConnectionAPIBase()->GetRegionDataManager().test_timezone_ =
        timezone;
  }

  void LoadCachedRegionData() {
    GetBraveVPNConnectionAPIBase()
        ->GetRegionDataManager()
        .LoadCachedRegionData();
  }

  void ClearRegions() {
    GetBraveVPNConnectionAPIBase()->GetRegionDataManager().regions_.clear();
  }

  bool NeedToUpdateRegionData() {
    return GetBraveVPNConnectionAPIBase()
        ->GetRegionDataManager()
        .NeedToUpdateRegionData();
  }

  mojom::Region device_region() {
    if (auto region_ptr =
            GetRegionPtrWithNameFromRegionList(GetBraveVPNConnectionAPIBase()
                                                   ->GetRegionDataManager()
                                                   .GetDeviceRegion(),
                                               regions())) {
      return *region_ptr;
    }
    return mojom::Region();
  }

  const std::vector<mojom::Region>& regions() {
    return GetBraveVPNConnectionAPIBase()->GetRegionDataManager().GetRegions();
  }

  PrefService* local_state() { return &local_pref_service_; }

  BraveVPNOSConnectionAPI* GetConnectionAPI() { return connection_api_.get(); }

 protected:
  TestingPrefServiceSimple local_pref_service_;
  network::TestURLLoaderFactory url_loader_factory_;
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<BraveVPNOSConnectionAPI> connection_api_;
};

TEST_F(BraveVPNOSConnectionAPIUnitTest, LoadRegionDataFromPrefsTest) {
  // Initially, prefs doesn't have region data.
  EXPECT_EQ(mojom::Region(), device_region());
  EXPECT_TRUE(regions().empty());

  // Set proper data to store them in prefs.
  OnFetchRegionList(GetRegionsData(), true);
  SetTestTimezone("Asia/Seoul");
  OnFetchTimezones(GetTimeZonesData(), true);

  // Check region data is set with above data.
  EXPECT_FALSE(mojom::Region() == device_region());
  EXPECT_FALSE(regions().empty());

  // Clear region data from api instance.
  ClearRegions();
  EXPECT_TRUE(regions().empty());

  // Check region data is loaded from prefs.
  LoadCachedRegionData();
  EXPECT_FALSE(regions().empty());
}

TEST_F(BraveVPNOSConnectionAPIUnitTest, RegionDataTest) {
  // Initially, prefs doesn't have region data.
  EXPECT_EQ(mojom::Region(), device_region());
  EXPECT_TRUE(regions().empty());

  // Test invalid region data.
  OnFetchRegionList(std::string(), true);
  EXPECT_TRUE(regions().empty());

  // Test valid region data parsing.
  OnFetchRegionList(GetRegionsData(), true);
  const size_t kRegionCount = 11;
  EXPECT_EQ(kRegionCount, regions().size());

  // First region in region list is set as a device region when fetch is failed.
  OnFetchTimezones(std::string(), false);
  EXPECT_EQ(regions()[0], device_region());

  // Test fallback region is replaced with proper device region
  // when valid timezone is used.
  // "asia-sg" region is used for "Asia/Seoul" tz.
  SetFallbackDeviceRegion();
  SetTestTimezone("Asia/Seoul");
  OnFetchTimezones(GetTimeZonesData(), true);
  EXPECT_EQ("asia-sg", device_region().name);

  // Test device region is not changed when invalid timezone is set.
  SetFallbackDeviceRegion();
  SetTestTimezone("Invalid");
  OnFetchTimezones(GetTimeZonesData(), true);
  EXPECT_EQ(regions()[0], device_region());

  // Test device region is not changed when invalid timezone is set.
  SetFallbackDeviceRegion();
  SetTestTimezone("Invalid");
  OnFetchTimezones(GetTimeZonesData(), true);
  EXPECT_EQ(regions()[0], device_region());
}

TEST_F(BraveVPNOSConnectionAPIUnitTest, NeedToUpdateRegionDataTest) {
  // Initially, need to update region data.
  EXPECT_TRUE(NeedToUpdateRegionData());

  // Still need to update.
  OnFetchRegionList(std::string(), true);
  EXPECT_TRUE(NeedToUpdateRegionData());

  // Don't need to update when got valid region data.
  OnFetchRegionList(GetRegionsData(), true);
  EXPECT_FALSE(NeedToUpdateRegionData());

  // Need to update again after 5h passed.
  task_environment_.AdvanceClock(base::Hours(5));
  EXPECT_TRUE(NeedToUpdateRegionData());
}

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
  // Prepare region data before asking connect.
  OnFetchRegionList(GetRegionsData(), true);

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
  // Prepare region data before asking connect.
  OnFetchRegionList(GetRegionsData(), true);

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

  test_api->connection_state_ = mojom::ConnectionState::CONNECTED;
  test_api->Connect();
  EXPECT_TRUE(test_api->needs_connect_);
  EXPECT_EQ(mojom::ConnectionState::DISCONNECTING,
            test_api->GetConnectionState());
  static_cast<BraveVPNOSConnectionAPISim*>(test_api)
      ->SetNetworkAvailableForTesting(false);
  test_api->OnDisconnected();
  EXPECT_TRUE(test_api->needs_connect_);
  static_cast<BraveVPNOSConnectionAPISim*>(test_api)
      ->SetNetworkAvailableForTesting(true);
  test_api->OnNetworkChanged(net::NetworkChangeNotifier::CONNECTION_ETHERNET);
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
  test_api->ResetConnectionInfo();
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

  // Test quick cancelled when |api_request_| is not null.
  // See the comment of BraveVPNOSConnectionAPIBase::api_request_.
  test_api->cancel_connecting_ = false;
  test_api->connection_state_ = mojom::ConnectionState::CONNECTING;
  // Explicitely create |api_request_|.
  test_api->GetAPIRequest();
  test_api->Disconnect();
  EXPECT_FALSE(test_api->cancel_connecting_);
  EXPECT_EQ(mojom::ConnectionState::DISCONNECTED,
            test_api->GetConnectionState());

  test_api->cancel_connecting_ = true;
  test_api->CreateVPNConnection();
  EXPECT_FALSE(test_api->cancel_connecting_);
  EXPECT_EQ(mojom::ConnectionState::DISCONNECTED, test_api->connection_state_);
}

// Ignore disconnected state change while connected. See the comment at
// BraveVPNOSConnectionAPI::UpdateAndNotifyConnectionStateChange().
TEST_F(BraveVPNOSConnectionAPIUnitTest,
       IgnoreDisconnectedStateWhileConnecting) {
  auto* test_api =
      static_cast<BraveVPNOSConnectionAPIBase*>(GetConnectionAPI());

  test_api->SetConnectionStateForTesting(mojom::ConnectionState::CONNECTING);
  test_api->UpdateAndNotifyConnectionStateChange(
      mojom::ConnectionState::DISCONNECTED);
  EXPECT_EQ(mojom::ConnectionState::CONNECTING, test_api->GetConnectionState());
}

TEST_F(BraveVPNOSConnectionAPIUnitTest,
       ClearLastConnectionErrorWhenNewConnectionStart) {
  auto* test_api =
      static_cast<BraveVPNOSConnectionAPIBase*>(GetConnectionAPI());

  // Prepare valid connection info.
  test_api->OnFetchHostnames("region-a", kHostNamesTestData, true);
  test_api->OnGetProfileCredentials(kProfileCredentialData, true);

  const std::string last_error = "Last error";
  test_api->SetLastConnectionError(last_error);
  EXPECT_EQ(last_error, test_api->GetLastConnectionError());
  test_api->Connect();
  EXPECT_TRUE(test_api->GetLastConnectionError().empty());
}

}  // namespace brave_vpn
