/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service_helper.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_manager.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_region_data_helper.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_region_data_manager.h"
#include "brave/components/brave_vpn/browser/connection/ikev2/connection_api_impl_sim.h"
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

class SystemVPNConnectionAPIUnitTest : public testing::Test {
 public:
  SystemVPNConnectionAPIUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    brave_vpn::RegisterLocalStatePrefs(local_pref_service_.registry());
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    connection_manager_ = std::make_unique<BraveVPNConnectionManager>(
        shared_url_loader_factory_, &local_pref_service_, base::NullCallback());
    connection_manager_->SetConnectionAPIImplForTesting(
        std::make_unique<ConnectionAPIImplSim>(connection_manager_.get(),
                                               shared_url_loader_factory_));
  }

  void OnFetchRegionList(const std::string& region_list, bool success) {
    GetBraveVPNConnectionManager()->GetRegionDataManager().OnFetchRegionList(
        region_list, success);
  }

  void OnFetchTimezones(const std::string& timezones_list, bool success) {
    GetBraveVPNConnectionManager()->GetRegionDataManager().OnFetchTimezones(
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

  // Use 6 countries data for test.
  std::string GetRegionsData() {
    return R"([
      {
        "cities": [
          {
            "continent": "Oceania",
            "country-iso-code": "AU",
            "latitude": 151.2070530275259,
            "longitude": -33.867749537753284,
            "name": "au-syd",
            "name-pretty": "Sydney",
            "region-precision": "city",
            "cities": [],
            "server-count": 8
          }
        ],
        "continent": "Oceania",
        "country-iso-code": "AU",
        "latitude": 133.79969396159765,
        "longitude": -23.62305911440252,
        "name": "ocn-aus",
        "name-pretty": "Australia",
        "region-precision": "country",
        "server-count": 8
      },
      {
        "cities": [
          {
            "continent": "Europe",
            "country-iso-code": "AT",
            "latitude": 16.361628116335655,
            "longitude": 48.20392172247492,
            "name": "eu-vie",
            "name-pretty": "Vienna",
            "region-precision": "city",
            "cities": [],
            "server-count": 10
          }
        ],
        "continent": "Europe",
        "country-iso-code": "AT",
        "latitude": 13.833811946421187,
        "longitude": 47.490394433887666,
        "name": "eu-at",
        "name-pretty": "Austria",
        "region-precision": "country",
        "server-count": 10
      },
      {
        "cities": [
          {
            "continent": "Europe",
            "country-iso-code": "BE",
            "latitude": 4.374847958682745,
            "longitude": 50.838778068842664,
            "name": "eu-bx",
            "name-pretty": "Brussels",
            "region-precision": "city",
            "cities": [],
            "server-count": 10
          }
        ],
        "continent": "Europe",
        "country-iso-code": "BE",
        "latitude": 4.733776325426172,
        "longitude": 50.712750850845715,
        "name": "eu-be",
        "name-pretty": "Belgium",
        "region-precision": "country",
        "server-count": 10
      },
      {
        "cities": [
          {
            "continent": "Asia",
            "country-iso-code": "SG",
            "latitude": 103.85019137019486,
            "longitude": 1.2900135414450815,
            "name": "sg-sg",
            "name-pretty": "Singapore",
            "region-precision": "city",
            "cities": [],
            "server-count": 10
          }
        ],
        "continent": "Asia",
        "country-iso-code": "SG",
        "latitude": 103.7967572191037,
        "longitude": 1.3827725407524207,
        "name": "asia-sg",
        "name-pretty": "Singapore",
        "region-precision": "country",
        "server-count": 10
      },
      {
        "cities": [
          {
            "continent": "South-America",
            "country-iso-code": "BR",
            "latitude": -46.63611733672991,
            "longitude": -23.547575340603583,
            "name": "sa-sao",
            "name-pretty": "Sao Paulo",
            "region-precision": "city",
            "cities": [],
            "server-count": 5
          }
        ],
        "continent": "South-America",
        "country-iso-code": "BR",
        "latitude": -48.99593985069093,
        "longitude": -12.240989380800045,
        "name": "sa-brz",
        "name-pretty": "Brazil",
        "region-precision": "country",
        "server-count": 5
      },
      {
        "cities": [
          {
            "continent": "North-America",
            "country-iso-code": "CA",
            "latitude": -79.39835761830456,
            "longitude": 43.7064997964195,
            "name": "ca-tor",
            "name-pretty": "Toronto",
            "region-precision": "city",
            "cities": [],
            "server-count": 5
          }
        ],
        "continent": "North-America",
        "country-iso-code": "CA",
        "latitude": -103.18476973580967,
        "longitude": 58.781368758466364,
        "name": "na-can",
        "name-pretty": "Canada",
        "region-precision": "country",
        "server-count": 5
      }])";
  }

  BraveVPNConnectionManager* GetBraveVPNConnectionManager() const {
    return connection_manager_.get();
  }

  void SetFallbackDeviceRegion() {
    GetBraveVPNConnectionManager()
        ->GetRegionDataManager()
        .SetFallbackDeviceRegion();
  }

  void SetTestTimezone(const std::string& timezone) {
    GetBraveVPNConnectionManager()->GetRegionDataManager().test_timezone_ =
        timezone;
  }

  void LoadCachedRegionData() {
    GetBraveVPNConnectionManager()->GetRegionDataManager().
        LoadCachedRegionData();
  }

  void ClearRegions() {
    GetBraveVPNConnectionManager()->GetRegionDataManager().regions_.clear();
  }

  bool NeedToUpdateRegionData() {
    return GetBraveVPNConnectionManager()
        ->GetRegionDataManager()
        .NeedToUpdateRegionData();
  }

  mojom::RegionPtr device_region() {
    const auto device_region_name = GetBraveVPNConnectionManager()
                                        ->GetRegionDataManager()
                                        .GetDeviceRegion();
    if (device_region_name.empty() || regions().empty()) {
      return nullptr;
    }

    if (auto region_ptr =
            GetRegionPtrWithNameFromRegionList(device_region_name, regions())) {
      return region_ptr.Clone();
    }

    return nullptr;
  }

  const std::vector<mojom::RegionPtr>& regions() {
    return GetBraveVPNConnectionManager()->GetRegionDataManager().GetRegions();
  }

  PrefService* local_state() { return &local_pref_service_; }

  ConnectionAPIImplSim* GetConnectionAPI() {
    return static_cast<ConnectionAPIImplSim*>(
        connection_manager_->connection_api_impl_.get());
  }

  SystemVPNConnectionAPIImplBase* GetSystemVPNConnectionAPI() {
    return static_cast<SystemVPNConnectionAPIImplBase*>(
        connection_manager_->connection_api_impl_.get());
  }

  BraveVPNConnectionInfo connection_info() {
    return GetSystemVPNConnectionAPI()->connection_info_;
  }

  void ResetConnectionInfo() {
    return GetSystemVPNConnectionAPI()->ResetConnectionInfo();
  }

 protected:
  TestingPrefServiceSimple local_pref_service_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<BraveVPNConnectionManager> connection_manager_;
};

TEST_F(SystemVPNConnectionAPIUnitTest, LoadRegionDataFromPrefsTest) {
  // Initially, prefs doesn't have region data.
  EXPECT_FALSE(device_region());
  EXPECT_TRUE(regions().empty());

  // Set proper data to store them in prefs.
  OnFetchRegionList(GetRegionsData(), true);
  SetTestTimezone("Asia/Seoul");
  OnFetchTimezones(GetTimeZonesData(), true);

  // Check region data is set with above data.
  EXPECT_TRUE(device_region());
  EXPECT_FALSE(regions().empty());

  // Clear region data from api instance.
  ClearRegions();
  EXPECT_TRUE(regions().empty());

  // Check region data is loaded from prefs.
  LoadCachedRegionData();
  EXPECT_FALSE(regions().empty());
}

TEST_F(SystemVPNConnectionAPIUnitTest, RegionDataTest) {
  // Initially, prefs doesn't have region data.
  EXPECT_FALSE(device_region());
  EXPECT_TRUE(regions().empty());

  // Test invalid region data.
  OnFetchRegionList(std::string(), true);
  EXPECT_TRUE(regions().empty());

  // Test valid region data parsing.
  OnFetchRegionList(GetRegionsData(), true);
  const size_t kRegionCount = 6;
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
  EXPECT_EQ("asia-sg", device_region()->name);

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

TEST_F(SystemVPNConnectionAPIUnitTest, NeedToUpdateRegionDataTest) {
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
TEST_F(SystemVPNConnectionAPIUnitTest,
       CreateOSVPNEntryWithValidInfoWhenConnectTest) {
  auto* test_api = GetConnectionAPI();
  test_api->CheckConnection();

  // Prepare valid connection info.
  test_api->OnFetchHostnames("eu-be", kHostNamesTestData, true);
  test_api->SetPreventCreationForTesting(true);
  test_api->OnGetProfileCredentials(kProfileCredentialData, true);
  EXPECT_TRUE(connection_info().IsValid());
  test_api->Connect();
  base::RunLoop().RunUntilIdle();
  // With cached connection info, connect process starts with
  // os vpn entry creation.

  EXPECT_TRUE(test_api->IsConnectionCreated());
}

TEST_F(SystemVPNConnectionAPIUnitTest, CreateOSVPNEntryWithInvalidInfoTest) {
  // Prepare region data before asking connect.
  OnFetchRegionList(GetRegionsData(), true);

  auto* test_api = GetConnectionAPI();
  test_api->CheckConnection();
  local_state()->SetString(prefs::kBraveVPNSelectedRegion, "eu-be");
  // Prepare valid connection info.
  test_api->OnFetchHostnames("eu-be", kHostNamesTestData, true);
  test_api->SetPreventCreationForTesting(true);
  test_api->OnGetProfileCredentials(kProfileCredentialData, true);
  ResetConnectionInfo();
  // W/o valid connection info, connect will not try to create
  // os vpn entry at the beginning.
  EXPECT_FALSE(connection_info().IsValid());
  test_api->Connect();
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(test_api->IsConnectionCreated());
}

TEST_F(SystemVPNConnectionAPIUnitTest, NeedsConnectTest) {
  // Prepare region data before asking connect.
  OnFetchRegionList(GetRegionsData(), true);

  auto* test_api = GetConnectionAPI();
  test_api->CheckConnection();

  // Check ignore Connect() request while connecting or disconnecting is
  // in-progress.
  local_state()->SetString(prefs::kBraveVPNSelectedRegion, "eu-be");
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
  GetSystemVPNConnectionAPI()->OnDisconnected();
  EXPECT_FALSE(test_api->needs_connect_);
  EXPECT_EQ(mojom::ConnectionState::CONNECTING, test_api->GetConnectionState());

  test_api->connection_state_ = mojom::ConnectionState::CONNECTED;
  test_api->Connect();
  EXPECT_TRUE(test_api->needs_connect_);
  EXPECT_EQ(mojom::ConnectionState::DISCONNECTING,
            test_api->GetConnectionState());
  test_api->SetNetworkAvailableForTesting(false);
  GetSystemVPNConnectionAPI()->OnDisconnected();
  EXPECT_TRUE(test_api->needs_connect_);
  test_api->SetNetworkAvailableForTesting(true);
  test_api->OnNetworkChanged(net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  EXPECT_FALSE(test_api->needs_connect_);
  EXPECT_EQ(mojom::ConnectionState::CONNECTING, test_api->GetConnectionState());
}

TEST_F(SystemVPNConnectionAPIUnitTest,
       CheckConnectionStateAfterNetworkStateChanged) {
  auto* test_api = GetConnectionAPI();
  EXPECT_FALSE(test_api->IsConnectionChecked());
  test_api->OnNetworkChanged(net::NetworkChangeNotifier::CONNECTION_WIFI);
  EXPECT_TRUE(test_api->IsConnectionChecked());
}

TEST_F(SystemVPNConnectionAPIUnitTest, HostnamesTest) {
  auto* test_api = GetConnectionAPI();
  // Set valid hostnames list
  test_api->hostname_.reset();
  test_api->OnFetchHostnames("eu-be", kHostNamesTestData, true);
  // Check best one is picked from fetched hostname list.
  EXPECT_EQ("host-2.brave.com", test_api->hostname_->hostname);

  // Can't get hostname from invalid hostnames list
  test_api->hostname_.reset();
  test_api->OnFetchHostnames("eu-be", "", false);
  EXPECT_FALSE(test_api->hostname_);
}

TEST_F(SystemVPNConnectionAPIUnitTest, ConnectionInfoTest) {
  auto* test_api = GetConnectionAPI();

  // Check valid connection info is set when valid hostname and profile
  // credential are fetched.
  test_api->connection_state_ = mojom::ConnectionState::CONNECTING;
  test_api->OnFetchHostnames("eu-be", kHostNamesTestData, true);
  EXPECT_EQ(mojom::ConnectionState::CONNECTING, test_api->GetConnectionState());

  // To prevent real os vpn entry creation.
  test_api->prevent_creation_ = true;
  test_api->OnGetProfileCredentials(kProfileCredentialData, true);
  EXPECT_EQ(mojom::ConnectionState::CONNECTING, test_api->GetConnectionState());
  EXPECT_TRUE(connection_info().IsValid());

  // Check cached connection info is cleared when user set new selected region.
  test_api->connection_state_ = mojom::ConnectionState::DISCONNECTED;
  ResetConnectionInfo();
  EXPECT_FALSE(connection_info().IsValid());

  // Fill connection info again.
  test_api->OnGetProfileCredentials(kProfileCredentialData, true);
  EXPECT_TRUE(connection_info().IsValid());

  // Check cached connection info is cleared when connect failed.
  test_api->OnConnectFailed();
  EXPECT_FALSE(connection_info().IsValid());
}

TEST_F(SystemVPNConnectionAPIUnitTest, CancelConnectingTest) {
  auto* test_api = GetConnectionAPI();

  GetConnectionAPI()->CheckConnection();

  test_api->cancel_connecting_ = true;
  test_api->connection_state_ = mojom::ConnectionState::CONNECTING;
  GetSystemVPNConnectionAPI()->OnCreated();
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
  GetSystemVPNConnectionAPI()->OnConnected();
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
  // See the comment of ConnectionAPIImpl::api_request_.
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
// ConnectionAPIImpl::UpdateAndNotifyConnectionStateChange().
TEST_F(SystemVPNConnectionAPIUnitTest,
       IgnoreDisconnectedStateWhileConnecting) {
  auto* test_api = GetConnectionAPI();

  test_api->SetConnectionStateForTesting(mojom::ConnectionState::CONNECTING);
  test_api->UpdateAndNotifyConnectionStateChange(
      mojom::ConnectionState::DISCONNECTED);
  EXPECT_EQ(mojom::ConnectionState::CONNECTING, test_api->GetConnectionState());
}

TEST_F(SystemVPNConnectionAPIUnitTest,
       ClearLastConnectionErrorWhenNewConnectionStart) {
  auto* test_api = GetConnectionAPI();

  // Prepare valid connection info.
  test_api->OnFetchHostnames("eu-be", kHostNamesTestData, true);
  test_api->OnGetProfileCredentials(kProfileCredentialData, true);

  const std::string last_error = "Last error";
  test_api->SetLastConnectionError(last_error);
  EXPECT_EQ(last_error, test_api->GetLastConnectionError());
  test_api->Connect();
  EXPECT_TRUE(test_api->GetLastConnectionError().empty());
}

}  // namespace brave_vpn
