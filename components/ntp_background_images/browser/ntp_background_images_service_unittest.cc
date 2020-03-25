/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/test/task_environment.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_utils.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "services/network/test/test_shared_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

const char kTestEmptyComponent[] = R"(
    {
        "schemaVersion": 1
    })";

  // Super referral wallpaper json data.
const char kTestSuperReferral[] = R"(
    {
      "schemaVersion": 1,
      "logo": {
        "imageUrl": "logo.png",
        "alt": "Technikke: For music lovers",
        "companyName": "Technikke",
        "destinationUrl": "https://www.brave.com/?from-super-referreer-demo"
      },
      "wallpapers": [
        {
          "imageUrl": "background-1.jpg",
          "focalPoint": { "x": 3988, "y": 2049}
        },
        {
          "imageUrl": "background-2.jpg",
          "focalPoint": { "x": 5233, "y": 3464}
        },
        {
          "imageUrl": "background-3.jpg"
        }
      ],
      "topSites": [
        {
          "name": "Brave",
          "destinationUrl": "https://brave.com/",
          "backgroundColor": "#e22919",
          "iconUrl": "brave.png"
        },
        {
          "name": "Wiki",
          "destinationUrl": "https://wikipedia.org/",
          "backgroundColor": "#e22919",
          "iconUrl": "wikipedia.png"
        },
        {
          "name": "BAT",
          "destinationUrl": "https://basicattentiontoken.org/",
          "backgroundColor": "#e22919",
          "iconUrl": "bat.png"
        }
      ]
    })";

class TestObserver : public NTPBackgroundImagesService::Observer {
 public:
  TestObserver() = default;
  ~TestObserver() override = default;

  void OnUpdated(NTPBackgroundImagesData* data) override {
    called_ = true;
    data_ = data;
  }

  NTPBackgroundImagesData* data_;
  bool called_ = false;
};

class TestNTPBackgroundImagesService : public NTPBackgroundImagesService {
 public:
  using NTPBackgroundImagesService::NTPBackgroundImagesService;

  void StartSuperReferralComponent(
      const std::string& super_referral_code) override {
    NTPBackgroundImagesService::StartSuperReferralComponent(
        super_referral_code);
    current_referral_code_ = super_referral_code;
    super_referral_component_started_ = true;
  }

  void StartSponsoredImagesComponent() override {
    NTPBackgroundImagesService::StartSponsoredImagesComponent();
    sponsored_images_component_started_ = true;
  }

  void DownloadSuperReferralMappingTable() override {
    NTPBackgroundImagesService::DownloadSuperReferralMappingTable();
    mapping_table_requested_ = true;
  }

  void MonitorReferralPromoCodeChange() override {
    NTPBackgroundImagesService::MonitorReferralPromoCodeChange();
    referral_promo_code_change_monitored_ = true;
  }

  void UnRegisterSuperReferralComponentIfRunning(
      const std::string& referral_code) override {
    NTPBackgroundImagesService::UnRegisterSuperReferralComponentIfRunning(
        referral_code);
    unregistered_referral_code_ = referral_code;
  }

  std::string current_referral_code_;
  std::string unregistered_referral_code_;
  bool super_referral_component_started_ = false;
  bool sponsored_images_component_started_ = false;
  bool mapping_table_requested_ = false;
  bool referral_promo_code_change_monitored_ = false;
};

class NTPBackgroundImagesServiceTest : public testing::Test {
 public:
  NTPBackgroundImagesServiceTest() {}

  void SetUp() override {
    auto* registry = pref_service_.registry();
    RegisterLocalStatePrefs(registry);
    brave::RegisterPrefsForBraveReferralsService(registry);
  }

  void Init() {
    service_.reset(new TestNTPBackgroundImagesService(
        nullptr, &pref_service_,
        base::MakeRefCounted<network::TestSharedURLLoaderFactory>()));
    service_->Init();
  }

  base::test::TaskEnvironment env_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<TestNTPBackgroundImagesService> service_;
};

TEST_F(NTPBackgroundImagesServiceTest, BasicTest) {
  Init();
  // By default manager doesn't have data.
  EXPECT_EQ(service_->GetBackgroundImagesData(), nullptr);
}

TEST_F(NTPBackgroundImagesServiceTest, InternalDataTest) {
  Init();
  TestObserver observer;
  service_->AddObserver(&observer);

  // Check with json file w/o schema version with empty object.
  service_->images_data_.reset();
  service_->OnGetComponentJsonData("{}");
  EXPECT_EQ(nullptr, service_->GetBackgroundImagesData());

  // Check with json file with empty object.
  service_->images_data_.reset();
  observer.called_ = false;
  observer.data_ = nullptr;
  service_->OnGetComponentJsonData(kTestEmptyComponent);
  auto* data = service_->GetBackgroundImagesData();
  EXPECT_EQ(data, nullptr);
  EXPECT_TRUE(observer.called_);
  EXPECT_TRUE(observer.data_->logo_alt_text.empty());

  const std::string test_json_string = R"(
      {
          "schemaVersion": 1,
          "logo": {
            "imageUrl":  "logo.png",
            "alt": "Technikke: For music lovers",
            "destinationUrl": "https://www.brave.com/",
            "companyName": "Technikke"
          },
          "wallpapers": [
              {
                "imageUrl": "background-1.jpg",
                "focalPoint": { "x": 696, "y": 691 }
              },
              {
                "imageUrl": "background-2.jpg"
              },
              {
                "imageUrl": "background-3.jpg",
                "focalPoint": {}
              }
          ]
      })";
  service_->images_data_.reset();
  observer.called_ = false;
  observer.data_ = nullptr;
  service_->OnGetComponentJsonData(test_json_string);
  data = service_->GetBackgroundImagesData();
  EXPECT_TRUE(data);
  EXPECT_TRUE(data->IsValid());
  EXPECT_FALSE(data->IsSuperReferral());
  // Above json data has 3 wallpapers.
  const size_t image_count = 3;
  EXPECT_EQ(image_count, data->backgrounds.size());
  EXPECT_EQ(696, data->backgrounds[0].focal_point.x());
  // Check default value is set if "focalPoint" is missed.
  EXPECT_EQ(0, data->backgrounds[1].focal_point.x());
  EXPECT_EQ(0, data->backgrounds[2].focal_point.x());
  EXPECT_TRUE(observer.called_);
  EXPECT_FALSE(observer.data_->logo_alt_text.empty());
  EXPECT_TRUE(*data->GetBackgroundAt(0).FindBoolKey("isSponsored"));

  // Invalid schema version
  const std::string test_json_string_higher_schema = R"(
    {
        "schemaVersion": 2,
        "logo": {
          "imageUrl":  "logo.png",
          "alt": "Technikke: For music lovers",
          "destinationUrl": "https://www.brave.com/",
          "companyName": "Technikke"
        },
        "wallpapers": [
            {
              "imageUrl": "background-1.jpg",
              "focalPoint": {}
            },
            {
              "imageUrl": "background-2.jpg",
              "focalPoint": {}
            },
            {
              "imageUrl": "background-3.jpg",
              "focalPoint": {}
            }
        ]
    })";
  service_->images_data_.reset();
  observer.called_ = false;
  observer.data_ = nullptr;
  service_->OnGetComponentJsonData(test_json_string_higher_schema);
  data = service_->GetBackgroundImagesData();
  EXPECT_FALSE(data);

  service_->images_data_.reset();
  observer.called_ = false;
  observer.data_ = nullptr;
  service_->OnGetComponentJsonData(kTestSuperReferral);
  data = service_->GetBackgroundImagesData();
  EXPECT_TRUE(data);
  const size_t wallpaper_count = 3;
  const size_t top_site_count = 3;
  EXPECT_EQ(wallpaper_count, data->wallpaper_image_urls().size());
  EXPECT_EQ(top_site_count, data->top_sites.size());
  EXPECT_TRUE(data->IsSuperReferral());
  EXPECT_FALSE(*data->GetBackgroundAt(0).FindBoolKey("isSponsored"));
  EXPECT_TRUE(observer.called_);

  service_->RemoveObserver(&observer);
}

// Linux doesn't support referral service now.
// So, always start NTP SI component.
#if defined(OS_LINUX)

TEST_F(NTPBackgroundImagesServiceTest, LinuxTest) {
  Init();

  EXPECT_TRUE(service_->sponsored_images_component_started_);
  EXPECT_FALSE(service_->mapping_table_requested_);
  EXPECT_FALSE(service_->referral_promo_code_change_monitored_);
  EXPECT_FALSE(service_->super_referral_component_started_);
}

#else
const char kTestMappingTable[] = R"(
    {
        "schemaVersion": 1,
        "BRV003": {
          "publicKey": "ABCDEFGHIJKLMN",
          "componentID": "abcdefghijklmn",
          "companyName": "Alphabet software"
        },
        "BRV004": {
          "publicKey": "1234567890",
          "componentID": "0123456789",
          "companyName": "Numeric software"
        }
    })";

// Test default referral code and first run.
// Sponsored Images component will be run after promo code set to pref.
TEST_F(NTPBackgroundImagesServiceTest, WithDefaultReferralCodeTest1) {
  Init();

  // Initially, mapping table is downloaded.
  EXPECT_TRUE(service_->mapping_table_requested_);
  EXPECT_FALSE(service_->referral_promo_code_change_monitored_);
  EXPECT_FALSE(service_->super_referral_component_started_);
  EXPECT_FALSE(service_->sponsored_images_component_started_);

  service_->OnGetMappingTableData(
      std::make_unique<std::string>(kTestMappingTable));

  // Referral code is not fetched from referrals service.
  // If current referral code is emoty, start SI component by default.
  // Promo code change monitoring is started.
  EXPECT_TRUE(service_->referral_promo_code_change_monitored_);
  EXPECT_TRUE(service_->sponsored_images_component_started_);
  EXPECT_FALSE(service_->super_referral_component_started_);

  // If default code is set, SI component is started.
  service_->sponsored_images_component_started_ = false;
  pref_service_.SetString(kReferralPromoCode, "BRV001");
  EXPECT_FALSE(service_->super_referral_component_started_);
  EXPECT_TRUE(service_->sponsored_images_component_started_);
}

// Test default referral code and not first run.
// Sponsored Images component will be run after getting mapping table.
TEST_F(NTPBackgroundImagesServiceTest, WithDefaultReferralCodeTest2) {
  pref_service_.SetString(kReferralPromoCode, "BRV001");
  Init();

  // Initially, mapping table is downloaded.
  EXPECT_TRUE(service_->mapping_table_requested_);
  EXPECT_FALSE(service_->referral_promo_code_change_monitored_);
  EXPECT_FALSE(service_->super_referral_component_started_);
  EXPECT_FALSE(service_->sponsored_images_component_started_);

  service_->OnGetMappingTableData(
      std::make_unique<std::string>(kTestMappingTable));

  // Check referral promo code is copied to cached promo code pref.
  EXPECT_EQ("BRV001",
            pref_service_.GetString(prefs::kNewTabPageCachedReferralPromoCode));

  // Default code is already set. SI is started.
  EXPECT_TRUE(service_->referral_promo_code_change_monitored_);
  EXPECT_TRUE(service_->sponsored_images_component_started_);
  EXPECT_FALSE(service_->super_referral_component_started_);
}

// Test non default referral code but it's not super referral.
// Sponsored Images component will be run after getting mapping table.
TEST_F(NTPBackgroundImagesServiceTest, WithNonSuperReferralCodeTest) {
  pref_service_.SetString(kReferralPromoCode, "BRV002");

  Init();
  // If non-default code is set, mapping table is requested.
  EXPECT_FALSE(service_->sponsored_images_component_started_);
  EXPECT_FALSE(service_->super_referral_component_started_);
  EXPECT_TRUE(service_->mapping_table_requested_);

  // If it's not super-referral, regional wallpaper is started.
  service_->OnGetMappingTableData(
      std::make_unique<std::string>(kTestMappingTable));
  EXPECT_TRUE(service_->sponsored_images_component_started_);
  EXPECT_FALSE(service_->super_referral_component_started_);
}

TEST_F(NTPBackgroundImagesServiceTest, WithSuperReferralCodeTest) {
  pref_service_.SetString(kReferralPromoCode, "BRV003");

  Init();

  // If it's super-referral, referral wallpaper is started.
  service_->OnGetMappingTableData(
      std::make_unique<std::string>(kTestMappingTable));
  EXPECT_TRUE(service_->super_referral_component_started_);
  EXPECT_FALSE(service_->sponsored_images_component_started_);

  // Got super referral component
  service_->OnGetComponentJsonData(kTestSuperReferral);
  auto* data = service_->GetBackgroundImagesData();
  EXPECT_TRUE(data->IsSuperReferral());

  // Simulate compaign ends and check sponsored images component is started.
  EXPECT_FALSE(service_->sponsored_images_component_started_);
  EXPECT_FALSE(pref_service_.FindPreference(
      prefs::kNewTabPageCachedReferralPromoCode)->IsDefaultValue());
  EXPECT_FALSE(pref_service_.FindPreference(
      prefs::kNewTabPageCachedSuperReferralComponentInfo)->IsDefaultValue());
  service_->OnGetComponentJsonData(kTestEmptyComponent);
  EXPECT_TRUE(service_->sponsored_images_component_started_);
  // Check cached referral promo code is cleared if campaign ends.
  EXPECT_TRUE(pref_service_.FindPreference(
      prefs::kNewTabPageCachedReferralPromoCode)->IsDefaultValue());
  EXPECT_TRUE(pref_service_.FindPreference(
      prefs::kNewTabPageCachedSuperReferralComponentInfo)->IsDefaultValue());
}

// Install super referral again and check previous install was unregistered
// and new SR is registered.
TEST_F(NTPBackgroundImagesServiceTest, InstallSuperReferralOverReferralTest) {
  pref_service_.SetString(kReferralPromoCode, "BRV004");

  Init();
  service_->OnGetMappingTableData(
      std::make_unique<std::string>(kTestMappingTable));
  EXPECT_TRUE(service_->super_referral_component_started_);
  EXPECT_FALSE(service_->sponsored_images_component_started_);
  EXPECT_EQ("BRV004", service_->current_referral_code_);

  service_->super_referral_component_started_ = false;
  pref_service_.SetString(kReferralPromoCode, "BRV003");
  EXPECT_TRUE(service_->super_referral_component_started_);
  EXPECT_EQ("BRV004", service_->unregistered_referral_code_);
  EXPECT_EQ("BRV003", service_->current_referral_code_);
}

TEST_F(NTPBackgroundImagesServiceTest, SimulateNetworkErrorTest) {
  Init();

  // If there is no cached super referral component info, NTP SI component is
  // registered.
  service_->OnGetMappingTableData(nullptr);
  EXPECT_TRUE(service_->sponsored_images_component_started_);
  EXPECT_FALSE(service_->super_referral_component_started_);

  // If there is cached super referral component info, NTP SR component is
  // registered.
  service_->sponsored_images_component_started_ = false;
  pref_service_.SetString(kReferralPromoCode, "BRV003");
  base::Value value(base::Value::Type::DICTIONARY);
  value.SetStringKey("publicKey", "ABCDEFGHIJKLMN");
  value.SetStringKey("componentID", "abcdefghijklmn");
  value.SetStringKey("companyName", "Brave software");
  pref_service_.Set(prefs::kNewTabPageCachedSuperReferralComponentInfo, value);

  service_->OnGetMappingTableData(nullptr);
  EXPECT_TRUE(service_->super_referral_component_started_);
  EXPECT_FALSE(service_->sponsored_images_component_started_);
}

TEST_F(NTPBackgroundImagesServiceTest, CleanUpTest) {
  Init();

  // If campaign ended component info is remained, clean it up.
  pref_service_.SetString(prefs::kNewTabPageCachedReferralPromoCode, "BRV010");
  base::Value value(base::Value::Type::DICTIONARY);
  value.SetStringKey("publicKey", "ABCDEFGHIJKLMN");
  value.SetStringKey("componentID", "abcdefghijklmn");
  value.SetStringKey("companyName", "Brave software");
  pref_service_.Set(prefs::kNewTabPageCachedSuperReferralComponentInfo, value);

  // NTP SI is started because BRV010 ref code is not super referral.
  service_->OnGetMappingTableData(
      std::make_unique<std::string>(kTestMappingTable));
  EXPECT_TRUE(service_->sponsored_images_component_started_);
  // Check cached info is cleared.
  EXPECT_TRUE(pref_service_.FindPreference(
      prefs::kNewTabPageCachedSuperReferralComponentInfo)->IsDefaultValue());
}

#endif  // OS_LINUX

}  // namespace ntp_background_images
