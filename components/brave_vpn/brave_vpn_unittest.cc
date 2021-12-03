/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/feature_list.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_vpn/brave_vpn_service_desktop.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#include "brave/components/brave_vpn/features.h"
#include "brave/components/brave_vpn/pref_names.h"
#include "brave/components/skus/browser/pref_names.h"
#include "brave/components/skus/browser/skus_sdk_context_impl.h"
#include "brave/components/skus/browser/skus_sdk_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/test/test_shared_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

class BraveVPNServiceTest : public testing::Test {
 private:
  std::unique_ptr<SkusSdkService> skus_service_;

 public:
  BraveVPNServiceTest() {
    scoped_feature_list_.InitAndEnableFeature(brave_vpn::features::kBraveVPN);
  }

  void SetUp() override {
    skus::SkusSdkContextImpl::RegisterProfilePrefs(pref_service_.registry());
    brave_vpn::prefs::RegisterProfilePrefs(pref_service_.registry());
    auto url_loader_factory =
        base::MakeRefCounted<network::TestSharedURLLoaderFactory>();
    skus_service_ =
        std::make_unique<SkusSdkService>(&pref_service_, url_loader_factory);
    service_ = std::make_unique<BraveVpnServiceDesktop>(
        url_loader_factory, &pref_service_, skus_service_.get());
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
  std::unique_ptr<BraveVpnServiceDesktop> service_;
};

TEST(BraveVPNFeatureTest, FeatureTest) {
  EXPECT_FALSE(brave_vpn::IsBraveVPNEnabled());
}

TEST_F(BraveVPNServiceTest, RegionDataTest) {
  // Test invalid region data.
  service_->OnFetchRegionList(std::string(), true);
  EXPECT_TRUE(service_->regions_.empty());

  // Test valid region data parsing.
  service_->OnFetchRegionList(GetRegionsData(), true);
  const size_t kRegionCount = 11;
  EXPECT_EQ(kRegionCount, service_->regions_.size());

  // First region in region list is set as a device region when fetch is failed.
  service_->OnFetchTimezones(std::string(), false);
  EXPECT_EQ(service_->regions_[0], service_->device_region_);

  // Test proper device region is set when valid timezone is used.
  // "asia-sg" region is used for "Asia/Seoul" tz.
  service_->device_region_ = brave_vpn::mojom::Region();
  service_->set_test_timezone("Asia/Seoul");
  service_->OnFetchTimezones(GetTimeZonesData(), true);
  EXPECT_EQ("asia-sg", service_->device_region_.name);

  // Test first region is set as a device region when invalid timezone is set.
  service_->device_region_ = brave_vpn::mojom::Region();
  service_->set_test_timezone("Invalid");
  service_->OnFetchTimezones(GetTimeZonesData(), true);
  EXPECT_EQ(service_->regions_[0], service_->device_region_);
}

TEST_F(BraveVPNServiceTest, HostnamesTest) {
  // Set valid hostnames list
  service_->hostname_.reset();
  service_->OnFetchHostnames("region-a", GetHostnamesData(), true);
  // Check best one is picked from fetched hostname list.
  EXPECT_EQ("host-2.brave.com", service_->hostname_->hostname);

  // Can't get hostname from invalid hostnames list
  service_->hostname_.reset();
  service_->OnFetchHostnames("invalid-region-b", "", false);
  EXPECT_FALSE(service_->hostname_);
}

// TODO(bsclifton): fix after flow is decided
TEST_F(BraveVPNServiceTest, DISABLED_LoadPurchasedStateTest) {
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, service_->purchased_state_);
  pref_service_.SetBoolean(skus::prefs::kSkusVPNHasCredential, true);
  EXPECT_EQ(PurchasedState::PURCHASED, service_->purchased_state_);
}

TEST_F(BraveVPNServiceTest, CancelConnectingTest) {
  service_->connection_state_ = ConnectionState::CONNECTING;
  service_->cancel_connecting_ = true;
  service_->connection_state_ = ConnectionState::CONNECTING;
  service_->OnCreated();
  EXPECT_FALSE(service_->cancel_connecting_);
  EXPECT_EQ(ConnectionState::DISCONNECTED, service_->connection_state_);

  // Start disconnect() when connect is done for cancelling.
  service_->cancel_connecting_ = false;
  service_->connection_state_ = ConnectionState::CONNECTING;
  service_->Disconnect();
  EXPECT_TRUE(service_->cancel_connecting_);
  EXPECT_EQ(ConnectionState::DISCONNECTING, service_->connection_state_);
  service_->OnConnected();
  EXPECT_FALSE(service_->cancel_connecting_);
  EXPECT_EQ(ConnectionState::DISCONNECTING, service_->connection_state_);

  service_->cancel_connecting_ = false;
  service_->connection_state_ = ConnectionState::CONNECTING;
  service_->Disconnect();
  EXPECT_TRUE(service_->cancel_connecting_);
  EXPECT_EQ(ConnectionState::DISCONNECTING, service_->connection_state_);

  service_->cancel_connecting_ = true;
  service_->CreateVPNConnection();
  EXPECT_FALSE(service_->cancel_connecting_);
  EXPECT_EQ(ConnectionState::DISCONNECTED, service_->connection_state_);

  service_->cancel_connecting_ = true;
  service_->connection_state_ = ConnectionState::CONNECTING;
  service_->OnFetchHostnames("", "", true);
  EXPECT_FALSE(service_->cancel_connecting_);
  EXPECT_EQ(ConnectionState::DISCONNECTED, service_->connection_state_);

  service_->cancel_connecting_ = true;
  service_->connection_state_ = ConnectionState::CONNECTING;
  service_->OnGetSubscriberCredential("", true);
  EXPECT_FALSE(service_->cancel_connecting_);
  EXPECT_EQ(ConnectionState::DISCONNECTED, service_->connection_state_);

  service_->cancel_connecting_ = true;
  service_->connection_state_ = ConnectionState::CONNECTING;
  service_->OnGetProfileCredentials("", true);
  EXPECT_FALSE(service_->cancel_connecting_);
  EXPECT_EQ(ConnectionState::DISCONNECTED, service_->connection_state_);
}

// TODO(bsclifton): fix after flow is decided
TEST_F(BraveVPNServiceTest, DISABLED_ConnectionInfoTest) {
  // Check valid connection info is set when valid hostname and profile
  // credential are fetched.
  service_->connection_state_ = ConnectionState::CONNECTING;
  pref_service_.SetBoolean(skus::prefs::kSkusVPNHasCredential, true);
  service_->OnFetchHostnames("region-a", GetHostnamesData(), true);
  EXPECT_EQ(ConnectionState::CONNECTING, service_->connection_state_);

  // To prevent real os vpn entry creation.
  service_->is_simulation_ = true;
  service_->OnGetProfileCredentials(GetProfileCredentialData(), true);
  EXPECT_EQ(ConnectionState::CONNECTING, service_->connection_state_);
  EXPECT_TRUE(service_->connection_info_.IsValid());

  // Check cached connection info is cleared when user set new selected region.
  service_->connection_state_ = ConnectionState::DISCONNECTED;
  brave_vpn::mojom::Region region;
  service_->SetSelectedRegion(region.Clone());
  EXPECT_FALSE(service_->connection_info_.IsValid());
}

TEST_F(BraveVPNServiceTest, NeedsConnectTest) {
  // Check ignore Connect() request while connecting or disconnecting is
  // in-progress.
  service_->connection_state_ = ConnectionState::CONNECTING;
  service_->Connect();
  EXPECT_EQ(ConnectionState::CONNECTING, service_->connection_state_);

  service_->connection_state_ = ConnectionState::DISCONNECTING;
  service_->Connect();
  EXPECT_EQ(ConnectionState::DISCONNECTING, service_->connection_state_);

  // Handle connect after disconnect current connection.
  service_->connection_state_ = ConnectionState::CONNECTED;
  service_->Connect();
  EXPECT_TRUE(service_->needs_connect_);
  EXPECT_EQ(ConnectionState::DISCONNECTING, service_->connection_state_);
  service_->OnDisconnected();
  EXPECT_FALSE(service_->needs_connect_);
  EXPECT_EQ(ConnectionState::CONNECTING, service_->connection_state_);
}

TEST_F(BraveVPNServiceTest, LoadRegionDataFromPrefsTest) {
  // Initially, prefs doesn't have region data.
  EXPECT_EQ(brave_vpn::mojom::Region(), service_->device_region_);
  EXPECT_TRUE(service_->regions_.empty());

  // Set proper data to store them in prefs.
  service_->OnFetchRegionList(GetRegionsData(), true);
  service_->set_test_timezone("Asia/Seoul");
  service_->OnFetchTimezones(GetTimeZonesData(), true);

  // Check region data is set with above data.
  EXPECT_FALSE(brave_vpn::mojom::Region() == service_->device_region_);
  EXPECT_FALSE(service_->regions_.empty());

  // Clear region data.
  service_->device_region_ = brave_vpn::mojom::Region();
  service_->regions_.clear();
  EXPECT_EQ(brave_vpn::mojom::Region(), service_->device_region_);
  EXPECT_TRUE(service_->regions_.empty());

  // Check region data is loaded from prefs.
  service_->LoadCachedRegionData();
  EXPECT_FALSE(brave_vpn::mojom::Region() == service_->device_region_);
  EXPECT_FALSE(service_->regions_.empty());
}
