/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/common/pref_names.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_referral_image_source.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_referral_images_data.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_referral_images_service.h"
#include "brave/components/ntp_sponsored_images/common/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_sponsored_images {

class TestObserver : public NTPReferralImagesService::Observer {
 public:
  TestObserver() = default;
  ~TestObserver() override = default;

  void OnReferralImagesUpdated(NTPReferralImagesData* data) override {
    called_ = true;
    data_ = data;
  }

  NTPReferralImagesData* data_ = nullptr;
  bool called_ = false;
};

class TestNTPReferralImagesService : public NTPReferralImagesService {
 public:
  using NTPReferralImagesService::NTPReferralImagesService;

  void RegisterReferralComponent() override {
    register_requested_ = true;
  }

 bool register_requested_ = false;
};

class NTPReferralImagesServiceTest : public testing::Test {
 public:
  NTPReferralImagesServiceTest() {}

  void SetUp() override {
    auto* registry = pref_service_.registry();
    NTPReferralImagesService::RegisterLocalStatePrefs(registry);
    brave::RegisterPrefsForBraveReferralsService(registry);
    service_.reset(new TestNTPReferralImagesService(nullptr, &pref_service_));
    service_->is_super_referral_ = true;
  }

  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<TestNTPReferralImagesService> service_;
};

TEST_F(NTPReferralImagesServiceTest, BasicTest) {
  // By default manager doesn't have valid data.
  EXPECT_FALSE(service_->GetReferralImagesData()->IsValid());
}

TEST_F(NTPReferralImagesServiceTest, InternalDataTest) {
  TestObserver observer;
  service_->AddObserver(&observer);

  // Check with json file w/o schema version with empty object.
  service_->images_data_.reset(new NTPReferralImagesData);
  service_->OnGetReferralJsonData(base::FilePath(), "{}");
  EXPECT_EQ(nullptr, service_->GetReferralImagesData());

  // Check with json file with empty object.
  const std::string test_empty_json_string = R"(
      {
          "schemaVersion": 1
      })";
  service_->images_data_.reset(new NTPReferralImagesData);
  observer.called_ = false;
  observer.data_ = nullptr;
  service_->OnGetReferralJsonData(base::FilePath(), test_empty_json_string);
  auto* data = service_->GetReferralImagesData();
  EXPECT_EQ(nullptr, data);
  EXPECT_TRUE(observer.called_);
  EXPECT_EQ(nullptr, observer.data_);

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
        ],
        "topSites": [
          {
            "name": "Shop at My Company",
            "destinationUrl": "https://www.company.com/shop",
            "iconUrl": "shop.png"
          }
        ]
    })";
  service_->images_data_.reset(new NTPReferralImagesData);
  service_->is_super_referral_ = true;
  observer.called_ = false;
  observer.data_ = nullptr;
  service_->OnGetReferralJsonData(base::FilePath(), test_json_string);
  data = service_->GetReferralImagesData();
  EXPECT_TRUE(data);
  EXPECT_TRUE(data->IsValid());
  // Above json data has 3 wallpapers.
  const size_t wallpaper_count = 3;
  const size_t top_site_count = 1;
  EXPECT_EQ(wallpaper_count, data->wallpaper_image_urls().size());
  EXPECT_EQ(top_site_count, data->top_sites.size());
  EXPECT_TRUE(observer.called_);
  EXPECT_FALSE(observer.data_->logo_alt_text.empty());

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
  service_->images_data_.reset(new NTPReferralImagesData);
  observer.called_ = false;
  observer.data_ = nullptr;
  service_->OnGetReferralJsonData(base::FilePath(),
                                test_json_string_higher_schema);
  data = service_->GetReferralImagesData();
  EXPECT_FALSE(data);

  service_->RemoveObserver(&observer);
}

TEST_F(NTPReferralImagesServiceTest, ImageSourceTest) {
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
        ],
        "topSites": [
          {
            "name": "Shop at My Company",
            "destinationUrl": "https://www.company.com/shop",
            "iconUrl": "shop.png"
          }
        ]
    })";
  service_->images_data_.reset(new NTPReferralImagesData);
  service_->OnGetReferralJsonData(base::FilePath(), test_json_string);
  auto* data = service_->GetReferralImagesData();
  EXPECT_TRUE(data);
  EXPECT_TRUE(data->IsValid());

  NTPReferralImageSource image_source(service_.get());
  EXPECT_TRUE(image_source.IsLogoPath("logo.png"));
  EXPECT_FALSE(image_source.IsLogoPath("logo1.png"));
  EXPECT_TRUE(image_source.IsIconPath("shop.png"));
  EXPECT_FALSE(image_source.IsIconPath("shop1.png"));
  // wallpaper file name pattern - wallpaper-N.jpg.
  EXPECT_TRUE(image_source.IsWallpaperPath("wallpaper-1.jpg"));
  EXPECT_FALSE(image_source.IsWallpaperPath("wallpaper-3.jpg"));
}

TEST_F(NTPReferralImagesServiceTest, MapperComponentTest) {
  // If referral code is default one, it isn't super referral.
  service_->is_super_referral_ = true;
  pref_service_.SetString(kReferralPromoCode, "BRV001");
  service_->OnPreferenceChanged();
  EXPECT_FALSE(service_->is_super_referral());

  const std::string test_mapping_table = R"(
    {
        "schemaVersion": 1,
        "BRV002": {
          "publicKey": "ABCDEFGHIJKLMN",
          "componentID": "abcdefghijklmn",
          "companyName": "Alphabet software"
        },
        "BRV003": {
          "publicKey": "1234567890",
          "componentID": "0123456789",
          "companyName": "Numeric software"
        }
    })";

  service_->is_super_referral_ = true;
  pref_service_.SetString(kReferralPromoCode, "BRV002");
  service_->OnGetMappingJsonData(test_mapping_table);
  EXPECT_EQ("ABCDEFGHIJKLMN",
            *pref_service_.Get(prefs::kReferralImagesServiceComponent)->
                FindStringKey("publicKey"));
  EXPECT_TRUE(service_->register_requested_);
  EXPECT_TRUE(service_->is_super_referral());

  // If current code is not in mapping table, it's not super referral.
  service_->is_super_referral_ = true;
  pref_service_.ClearPref(prefs::kReferralImagesServiceComponent);
  service_->register_requested_ = false;
  pref_service_.SetString(kReferralPromoCode, "BRV007");
  service_->OnGetMappingJsonData(test_mapping_table);
  EXPECT_EQ(nullptr,
            pref_service_.Get(prefs::kReferralImagesServiceComponent)->
                FindStringKey("publicKey"));
  EXPECT_FALSE(service_->register_requested_);
  EXPECT_FALSE(service_->is_super_referral());

}

}  // namespace ntp_sponsored_images
