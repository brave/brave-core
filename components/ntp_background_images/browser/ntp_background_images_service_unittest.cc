/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/test/task_environment.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_referrals/buildflags/buildflags.h"
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

  void CheckSuperReferralComponent() override {
    NTPBackgroundImagesService::CheckSuperReferralComponent();
    checked_super_referral_component_ = true;
  }

  void RegisterSponsoredImagesComponent() override {
    NTPBackgroundImagesService::RegisterSponsoredImagesComponent();
    sponsored_images_component_started_ = true;
  }

  void RegisterSuperReferralComponent() override {
    NTPBackgroundImagesService::RegisterSuperReferralComponent();
    super_referral_component_started_ = true;
  }

  void DownloadSuperReferralMappingTable() override {
    NTPBackgroundImagesService::DownloadSuperReferralMappingTable();
    mapping_table_requested_ = true;
  }

  void MonitorReferralPromoCodeChange() override {
    NTPBackgroundImagesService::MonitorReferralPromoCodeChange();
    referral_promo_code_change_monitored_ = true;
  }

  void MarkThisInstallIsNotSuperReferralForever() override {
    NTPBackgroundImagesService::MarkThisInstallIsNotSuperReferralForever();
    marked_this_install_is_not_super_referral_forever_ = true;
  }

  void UnRegisterSuperReferralComponent() override {
    NTPBackgroundImagesService::UnRegisterSuperReferralComponent();
    unregistered_super_referral_component_ = true;
  }

  bool super_referral_component_started_ = false;
  bool checked_super_referral_component_ = false;
  bool sponsored_images_component_started_ = false;
  bool mapping_table_requested_ = false;
  bool referral_promo_code_change_monitored_ = false;
  bool marked_this_install_is_not_super_referral_forever_ = false;
  bool unregistered_super_referral_component_ = false;
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
        nullptr, &pref_service_, base::FilePath(),
        base::MakeRefCounted<network::TestSharedURLLoaderFactory>()));
    service_->Init();
  }

  base::test::TaskEnvironment env_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<TestNTPBackgroundImagesService> service_;
};

TEST_F(NTPBackgroundImagesServiceTest, BasicTest) {
  Init();
  // NTP SI Component is registered always at start.
  EXPECT_TRUE(service_->sponsored_images_component_started_);
}

TEST_F(NTPBackgroundImagesServiceTest, InternalDataTest) {
  Init();
  TestObserver observer;
  service_->AddObserver(&observer);

  // Check with json file w/o schema version with empty object.
  service_->si_images_data_.reset();
  service_->OnGetComponentJsonData(false, "{}");
  EXPECT_EQ(nullptr, service_->GetBackgroundImagesData(false));

  // Check with json file with empty object.
  service_->si_images_data_.reset();
  observer.called_ = false;
  observer.data_ = nullptr;
  service_->OnGetComponentJsonData(false, kTestEmptyComponent);
  auto* data = service_->GetBackgroundImagesData(false);
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
  service_->si_images_data_.reset();
  observer.called_ = false;
  observer.data_ = nullptr;
  service_->OnGetComponentJsonData(false, test_json_string);
  data = service_->GetBackgroundImagesData(false);
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
  service_->si_images_data_.reset();
  observer.called_ = false;
  observer.data_ = nullptr;
  service_->OnGetComponentJsonData(false, test_json_string_higher_schema);
  data = service_->GetBackgroundImagesData(false);
  EXPECT_FALSE(data);

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

#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
const char kTestMappingTable[] = R"(
    {
        "schemaVersion": 1,
        "BRV003": {
          "publicKey": "ABCDEFGHIJKLMN",
          "componentID": "abcdefghijklmn",
          "themeName": "Alphabet software"
        },
        "BRV004": {
          "publicKey": "1234567890",
          "componentID": "0123456789",
          "themeName": "Numeric software"
        }
    })";

  // Super referral wallpaper json data.
const char kTestSuperReferral[] = R"(
    {
      "schemaVersion": 1,
      "themeName": "Technikke",
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

TEST_F(NTPBackgroundImagesServiceTest, BasicSuperReferralTest) {
  Init();
  TestObserver observer;
  service_->AddObserver(&observer);

  service_->sr_images_data_.reset();
  observer.called_ = false;
  observer.data_ = nullptr;
  service_->OnGetComponentJsonData(true, kTestSuperReferral);
  auto* data = service_->GetBackgroundImagesData(true);
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

// Test default referral code and first run.
// Sponsored Images component will be run after promo code set to pref.
TEST_F(NTPBackgroundImagesServiceTest, WithDefaultReferralCodeTest1) {
  Init();

  // Initially, only SI is started and pref is monitored to get referral code.
  EXPECT_TRUE(service_->sponsored_images_component_started_);
  EXPECT_TRUE(service_->referral_promo_code_change_monitored_);
  EXPECT_TRUE(service_->checked_super_referral_component_);
  EXPECT_FALSE(service_->mapping_table_requested_);
  EXPECT_FALSE(service_->super_referral_component_started_);
  EXPECT_FALSE(service_->marked_this_install_is_not_super_referral_forever_);

  // If default code is set, SI component is started.
  pref_service_.SetString(kReferralPromoCode, "BRV001");
  EXPECT_TRUE(service_->marked_this_install_is_not_super_referral_forever_);
}

// Test default referral code and not first run.
// Sponsored Images component will be run after getting mapping table.
TEST_F(NTPBackgroundImagesServiceTest, WithDefaultReferralCodeTest2) {
  pref_service_.SetString(kReferralPromoCode, "BRV001");
  pref_service_.Set(prefs::kNewTabPageCachedSuperReferralComponentInfo,
                    base::Value(base::Value::Type::DICTIONARY));
  Init();

  // Initially, SI is started and SR checking is done.
  // This will not monitor prefs change because we already marked this is not
  // the super referral.
  EXPECT_TRUE(service_->sponsored_images_component_started_);
  EXPECT_TRUE(service_->checked_super_referral_component_);
  EXPECT_FALSE(service_->mapping_table_requested_);
  EXPECT_FALSE(service_->referral_promo_code_change_monitored_);
  EXPECT_FALSE(service_->super_referral_component_started_);
}

// Test non default referral code but it's not super referral.
// Sponsored Images component will be run after getting mapping table.
TEST_F(NTPBackgroundImagesServiceTest, WithNonSuperReferralCodeTest) {
  Init();

  EXPECT_TRUE(service_->sponsored_images_component_started_);
  EXPECT_TRUE(service_->checked_super_referral_component_);
  EXPECT_TRUE(service_->referral_promo_code_change_monitored_);
  EXPECT_FALSE(service_->mapping_table_requested_);
  EXPECT_FALSE(service_->super_referral_component_started_);

  pref_service_.SetString(kReferralPromoCode, "BRV002");

  // Mapping table is requested because it's not a default code.
  EXPECT_TRUE(service_->mapping_table_requested_);
  EXPECT_FALSE(service_->marked_this_install_is_not_super_referral_forever_);

  // If it's not super-referral, we mark this install is not a valid SR.
  service_->OnGetMappingTableData(
      std::make_unique<std::string>(kTestMappingTable));

  EXPECT_TRUE(service_->marked_this_install_is_not_super_referral_forever_);
  EXPECT_FALSE(service_->super_referral_component_started_);
}

TEST_F(NTPBackgroundImagesServiceTest, WithSuperReferralCodeTest) {
  Init();

  EXPECT_TRUE(service_->sponsored_images_component_started_);
  EXPECT_TRUE(service_->checked_super_referral_component_);
  EXPECT_TRUE(service_->referral_promo_code_change_monitored_);
  EXPECT_FALSE(service_->mapping_table_requested_);
  EXPECT_FALSE(service_->super_referral_component_started_);

  pref_service_.SetString(kReferralPromoCode, "BRV003");

  // Mapping table is requested because it's not a default code.
  EXPECT_TRUE(service_->mapping_table_requested_);
  EXPECT_FALSE(service_->marked_this_install_is_not_super_referral_forever_);

  service_->OnGetMappingTableData(
      std::make_unique<std::string>(kTestMappingTable));

  // This is super referral code. So, start SR component.
  EXPECT_TRUE(service_->super_referral_component_started_);
  EXPECT_FALSE(service_->marked_this_install_is_not_super_referral_forever_);

  // Got super referral component
  service_->OnGetComponentJsonData(true, kTestSuperReferral);
  auto* data = service_->GetBackgroundImagesData(true);
  EXPECT_TRUE(data->IsSuperReferral());

  EXPECT_FALSE(pref_service_.FindPreference(
      prefs::kNewTabPageCachedSuperReferralComponentInfo)->IsDefaultValue());
  service_->OnGetComponentJsonData(true, kTestEmptyComponent);

  EXPECT_TRUE(service_->marked_this_install_is_not_super_referral_forever_);
  EXPECT_TRUE(service_->unregistered_super_referral_component_);
}

#endif  // BUILDFLAG(ENABLE_BRAVE_REFERRALS)

#endif  // OS_LINUX

}  // namespace ntp_background_images
