/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/feature_list.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_vpn/brave_vpn_service.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#include "brave/components/brave_vpn/features.h"
#include "brave/components/brave_vpn/pref_names.h"
#include "brave/components/skus/browser/pref_names.h"
#include "brave/components/skus/browser/skus_context_impl.h"
#include "brave/components/skus/browser/skus_service_impl.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/features.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/browser_context.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/test/test_shared_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

using ConnectionState = brave_vpn::mojom::ConnectionState;
using PurchasedState = brave_vpn::mojom::PurchasedState;

namespace {

std::unique_ptr<skus::SkusServiceImpl> skus_service_;
mojo::PendingRemote<skus::mojom::SkusService> GetSkusService() {
  if (!skus_service_) {
    return mojo::PendingRemote<skus::mojom::SkusService>();
  }
  return static_cast<skus::SkusServiceImpl*>(skus_service_.get())->MakeRemote();
}

}  // namespace

class BraveVPNServiceTest : public testing::Test {
 public:
  BraveVPNServiceTest() {
    scoped_feature_list_.InitWithFeatures(
        {skus::features::kSkusFeature, brave_vpn::features::kBraveVPN}, {});
  }

  void SetUp() override {
    skus::RegisterProfilePrefs(pref_service_.registry());
    brave_vpn::prefs::RegisterProfilePrefs(pref_service_.registry());

    // Setup required for SKU (dependency of VPN)
    auto url_loader_factory =
        base::MakeRefCounted<network::TestSharedURLLoaderFactory>();
    skus_service_ = std::make_unique<skus::SkusServiceImpl>(&pref_service_,
                                                            url_loader_factory);
    auto callback = base::BindRepeating(GetSkusService);
    service_ = std::make_unique<BraveVpnService>(url_loader_factory,
                                                 &pref_service_, callback);
  }

  void OnFetchRegionList(bool background_fetch,
                         const std::string& region_list,
                         bool success) {
    service_->OnFetchRegionList(background_fetch, region_list, success);
  }

  void OnFetchTimezones(const std::string& timezones_list, bool success) {
    service_->OnFetchTimezones(timezones_list, success);
  }

  void OnFetchHostnames(const std::string& region,
                        const std::string& hostnames,
                        bool success) {
    service_->OnFetchHostnames(region, hostnames, success);
  }

  std::vector<brave_vpn::mojom::Region>& regions() const {
    return service_->regions_;
  }

  brave_vpn::mojom::Region& device_region() const {
    return service_->device_region_;
  }

  std::unique_ptr<brave_vpn::Hostname>& hostname() {
    return service_->hostname_;
  }

  bool& cancel_connecting() { return service_->cancel_connecting_; }

  ConnectionState& connection_state() { return service_->connection_state_; }

  PurchasedState& purchased_state() { return service_->purchased_state_; }

  bool& is_simulation() { return service_->is_simulation_; }

  bool& needs_connect() { return service_->needs_connect_; }

  void Connect() { service_->Connect(); }

  void Disconnect() { service_->Disconnect(); }

  void CreateVPNConnection() { service_->CreateVPNConnection(); }

  void LoadCachedRegionData() { service_->LoadCachedRegionData(); }

  void OnCreated() { service_->OnCreated(); }

  void OnGetSubscriberCredentialV12(const std::string& subscriber_credential,
                                    bool success) {
    service_->OnGetSubscriberCredentialV12(subscriber_credential, success);
  }

  void OnGetProfileCredentials(const std::string& profile_credential,
                               bool success) {
    service_->OnGetProfileCredentials(profile_credential, success);
  }

  void OnConnected() { service_->OnConnected(); }

  void OnDisconnected() { service_->OnConnected(); }

  const brave_vpn::BraveVPNConnectionInfo& GetConnectionInfo() {
    return service_->GetConnectionInfo();
  }

  void ResetDeviceRegion() {
    service_->device_region_ = brave_vpn::mojom::Region();
  }

  void SetTestTimezone(const std::string& timezone) {
    service_->test_timezone_ = timezone;
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

  std::string GetHostnamesData() {
    return R"([
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
  }

  std::string GetProfileCredentialData() {
    return R"(
        {
          "eap-username": "brave-user",
          "eap-password": "brave-pwd"
        }
      )";
  }

  base::test::ScopedFeatureList scoped_feature_list_;
  content::BrowserTaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  std::unique_ptr<BraveVpnService> service_;
};

// TODO(bsclifton): re-enable test after figuring out why crash is happening
TEST(BraveVPNFeatureTest, DISABLED_FeatureTest) {
  EXPECT_FALSE(brave_vpn::IsBraveVPNEnabled());
}

// TODO(bsclifton): re-enable test after figuring out why crash is happening
TEST_F(BraveVPNServiceTest, DISABLED_RegionDataTest) {
  // Test invalid region data.
  OnFetchRegionList(false, std::string(), true);
  EXPECT_TRUE(regions().empty());

  // Test valid region data parsing.
  OnFetchRegionList(false, GetRegionsData(), true);
  const size_t kRegionCount = 11;
  EXPECT_EQ(kRegionCount, regions().size());

  // First region in region list is set as a device region when fetch is failed.
  OnFetchTimezones(std::string(), false);
  EXPECT_EQ(regions()[0], device_region());

  // Test proper device region is set when valid timezone is used.
  // "asia-sg" region is used for "Asia/Seoul" tz.
  ResetDeviceRegion();
  SetTestTimezone("Asia/Seoul");
  OnFetchTimezones(GetTimeZonesData(), true);
  EXPECT_EQ("asia-sg", device_region().name);

  // Test first region is set as a device region when invalid timezone is set.
  ResetDeviceRegion();
  SetTestTimezone("Invalid");
  OnFetchTimezones(GetTimeZonesData(), true);
  EXPECT_EQ(regions()[0], device_region());
}

TEST_F(BraveVPNServiceTest, DISABLED_HostnamesTest) {
  // Set valid hostnames list
  hostname().reset();
  OnFetchHostnames("region-a", GetHostnamesData(), true);
  // Check best one is picked from fetched hostname list.
  EXPECT_EQ("host-2.brave.com", hostname()->hostname);

  // Can't get hostname from invalid hostnames list
  hostname().reset();
  OnFetchHostnames("invalid-region-b", "", false);
  EXPECT_FALSE(hostname());
}

// TODO(bsclifton): fix after flow is decided
TEST_F(BraveVPNServiceTest, DISABLED_LoadPurchasedStateTest) {
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, purchased_state());
  EXPECT_EQ(PurchasedState::PURCHASED, purchased_state());
}

// TODO(bsclifton): re-enable test after figuring out why crash is happening
TEST_F(BraveVPNServiceTest, DISABLED_CancelConnectingTest) {
  cancel_connecting() = true;
  connection_state() = ConnectionState::CONNECTING;
  OnCreated();
  EXPECT_FALSE(cancel_connecting());
  EXPECT_EQ(ConnectionState::DISCONNECTED, connection_state());

  // Start disconnect() when connect is done for cancelling.
  cancel_connecting() = false;
  connection_state() = ConnectionState::CONNECTING;
  Disconnect();
  EXPECT_TRUE(cancel_connecting());
  EXPECT_EQ(ConnectionState::DISCONNECTING, connection_state());
  OnConnected();
  EXPECT_FALSE(cancel_connecting());
  EXPECT_EQ(ConnectionState::DISCONNECTING, connection_state());

  cancel_connecting() = false;
  connection_state() = ConnectionState::CONNECTING;
  Disconnect();
  EXPECT_TRUE(cancel_connecting());
  EXPECT_EQ(ConnectionState::DISCONNECTING, connection_state());

  cancel_connecting() = true;
  CreateVPNConnection();
  EXPECT_FALSE(cancel_connecting());
  EXPECT_EQ(ConnectionState::DISCONNECTED, connection_state());

  cancel_connecting() = true;
  connection_state() = ConnectionState::CONNECTING;
  OnFetchHostnames("", "", true);
  EXPECT_FALSE(cancel_connecting());
  EXPECT_EQ(ConnectionState::DISCONNECTED, connection_state());

  cancel_connecting() = true;
  connection_state() = ConnectionState::CONNECTING;
  OnGetSubscriberCredentialV12("", true);
  EXPECT_FALSE(cancel_connecting());
  EXPECT_EQ(ConnectionState::DISCONNECTED, connection_state());

  cancel_connecting() = true;
  connection_state() = ConnectionState::CONNECTING;
  OnGetProfileCredentials("", true);
  EXPECT_FALSE(cancel_connecting());
  EXPECT_EQ(ConnectionState::DISCONNECTED, connection_state());
}

// TODO(bsclifton): fix after flow is decided
TEST_F(BraveVPNServiceTest, DISABLED_ConnectionInfoTest) {
  // Check valid connection info is set when valid hostname and profile
  // credential are fetched.
  connection_state() = ConnectionState::CONNECTING;
  OnFetchHostnames("region-a", GetHostnamesData(), true);
  EXPECT_EQ(ConnectionState::CONNECTING, connection_state());

  // To prevent real os vpn entry creation.
  is_simulation() = true;
  OnGetProfileCredentials(GetProfileCredentialData(), true);
  EXPECT_EQ(ConnectionState::CONNECTING, connection_state());
  EXPECT_TRUE(GetConnectionInfo().IsValid());

  // Check cached connection info is cleared when user set new selected region.
  connection_state() = ConnectionState::DISCONNECTED;
  service_->SetSelectedRegion(brave_vpn::mojom::Region().Clone());
  EXPECT_FALSE(GetConnectionInfo().IsValid());
}

// TODO(bsclifton): re-enable test after figuring out why crash is happening
TEST_F(BraveVPNServiceTest, DISABLED_NeedsConnectTest) {
  // Check ignore Connect() request while connecting or disconnecting is
  // in-progress.
  connection_state() = ConnectionState::CONNECTING;
  Connect();
  EXPECT_EQ(ConnectionState::CONNECTING, connection_state());

  connection_state() = ConnectionState::DISCONNECTING;
  Connect();
  EXPECT_EQ(ConnectionState::DISCONNECTING, connection_state());

  // Handle connect after disconnect current connection.
  connection_state() = ConnectionState::CONNECTED;
  Connect();
  EXPECT_TRUE(needs_connect());
  EXPECT_EQ(ConnectionState::DISCONNECTING, connection_state());
  OnDisconnected();
  EXPECT_FALSE(needs_connect());
  EXPECT_EQ(ConnectionState::CONNECTING, connection_state());
}

// TODO(bsclifton): re-enable test after figuring out why crash is happening
TEST_F(BraveVPNServiceTest, DISABLED_LoadRegionDataFromPrefsTest) {
  // Initially, prefs doesn't have region data.
  EXPECT_EQ(brave_vpn::mojom::Region(), device_region());
  EXPECT_TRUE(regions().empty());

  // Set proper data to store them in prefs.
  OnFetchRegionList(false, GetRegionsData(), true);
  SetTestTimezone("Asia/Seoul");
  OnFetchTimezones(GetTimeZonesData(), true);

  // Check region data is set with above data.
  EXPECT_FALSE(brave_vpn::mojom::Region() == device_region());
  EXPECT_FALSE(regions().empty());

  // Clear region data.
  device_region() = brave_vpn::mojom::Region();
  regions().clear();
  EXPECT_EQ(brave_vpn::mojom::Region(), device_region());
  EXPECT_TRUE(regions().empty());

  // Check region data is loaded from prefs.
  LoadCachedRegionData();
  EXPECT_FALSE(brave_vpn::mojom::Region() == device_region());
  EXPECT_FALSE(regions().empty());
}
