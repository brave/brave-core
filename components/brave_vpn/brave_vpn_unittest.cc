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
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/test/test_shared_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

class BraveVPNTest : public testing::Test {
 public:
  BraveVPNTest() {
    scoped_feature_list_.InitAndEnableFeature(brave_vpn::features::kBraveVPN);
  }

  void SetUp() override {
    service_ = std::make_unique<BraveVpnServiceDesktop>(
        base::MakeRefCounted<network::TestSharedURLLoaderFactory>(),
        &pref_service_);
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

  base::test::ScopedFeatureList scoped_feature_list_;
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<BraveVpnServiceDesktop> service_;
};

TEST(BraveVPNFeatureTest, FeatureTest) {
  EXPECT_FALSE(brave_vpn::IsBraveVPNEnabled());
}

TEST_F(BraveVPNTest, RegionDataTest) {
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

TEST_F(BraveVPNTest, HostnamesTest) {
  // Set valid hostnames list
  service_->OnFetchHostnames("region-a", GetHostnamesData(), true);
  EXPECT_EQ(5UL, service_->hostnames_["region-a"].size());

  // Set invalid hostnames list
  service_->OnFetchHostnames("invalid-region-b", "", false);
  EXPECT_EQ(0UL, service_->hostnames_["invalid-region-b"].size());
}
