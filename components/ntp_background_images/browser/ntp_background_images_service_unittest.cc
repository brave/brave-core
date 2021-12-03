/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/test/task_environment.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/brave_referrals/buildflags/buildflags.h"
#include "brave/components/brave_referrals/common/pref_names.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

constexpr char kTestEmptyComponent[] = R"(
    {
        "schemaVersion": 1
    })";

constexpr char kTestSponsoredImages[] = R"(
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
              "imageUrl": "background-2.jpg",
              "logo": {
                "imageUrl": "logo-2.png",
                "alt": "logo2",
                "companyName": "BAT",
                "destinationUrl": "https://www.bat.com/"
              }
            },
            {
              "imageUrl": "background-3.jpg",
              "focalPoint": {}
            }
        ]
    })";

constexpr char kTestSponsoredImagesWithMultipleCampaigns[] = R"(
    {
        "schemaVersion": 1,
        "campaigns": [
          {
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
                  "imageUrl": "background-2.jpg",
                  "logo": {
                    "imageUrl": "logo-2.png",
                    "alt": "logo2",
                    "companyName": "BAT",
                    "destinationUrl": "https://www.bat.com/"
                  }
                },
                {
                  "imageUrl": "background-3.jpg",
                  "focalPoint": {}
                }
            ]
          },
          {
            "logo": {
              "imageUrl":  "logo-3.png",
              "alt": "Technikke: For music lovers",
              "destinationUrl": "https://www.brave.com/",
              "companyName": "Technikke"
            },
            "wallpapers": [
                {
                  "imageUrl": "background-4.jpg",
                  "focalPoint": { "x": 696, "y": 691 }
                },
                {
                  "imageUrl": "background-5.jpg",
                  "logo": {
                    "imageUrl": "logo-4.png",
                    "alt": "logo3",
                    "companyName": "BAT",
                    "destinationUrl": "https://www.bat.com/"
                  }
                }
            ]
          }
        ]
    })";

constexpr char kTestBackgroundImages[] = R"(
    {
      "schemaVersion": 1,
      "images": [
        {
          "name": "ntp-2020/2021-1",
          "source": "background-image-source.webp",
          "author": "Brave Software",
          "link": "https://brave.com/",
          "originalUrl": "Contributor sent the hi-res version through email",
          "license": "https://brave.com/about/"
        },
        {
          "name": "ntp-2020/2021-2",
          "source": "background-image-source.avif",
          "author": "Brave Software",
          "link": "https://brave.com/",
          "originalUrl": "Contributor sent the hi-res version through email",
          "license": "https://brave.com/about/"
        }
      ]
    })";

class TestObserver : public NTPBackgroundImagesService::Observer {
 public:
  TestObserver() = default;
  ~TestObserver() override = default;

  void OnUpdated(NTPBackgroundImagesData* data) override {
    on_bi_updated_ = true;
    bi_data_ = data;
  }

  void OnUpdated(NTPSponsoredImagesData* data) override {
    on_si_updated_ = true;
    si_data_ = data;
  }
  void OnSuperReferralEnded() override {
    on_super_referral_ended_ = true;
  }

  raw_ptr<NTPBackgroundImagesData> bi_data_ = nullptr;
  bool on_bi_updated_ = false;
  raw_ptr<NTPSponsoredImagesData> si_data_ = nullptr;
  bool on_si_updated_ = false;
  bool on_super_referral_ended_ = false;
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

  void RegisterBackgroundImagesComponent() override {
    NTPBackgroundImagesService::RegisterBackgroundImagesComponent();
    background_images_component_started_ = true;
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
  bool background_images_component_started_ = false;
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
    NTPBackgroundImagesService::RegisterLocalStatePrefs(registry);
    brave::RegisterPrefsForBraveReferralsService(registry);
  }

  void Init() {
    service_.reset(new TestNTPBackgroundImagesService(nullptr, &pref_service_));
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
  // If ENABLE_NTP_BACKGROUND_IMAGES then BI shall be registered
  EXPECT_TRUE(service_->background_images_component_started_);
}

TEST_F(NTPBackgroundImagesServiceTest, InternalDataTest) {
  Init();
  TestObserver observer;
  service_->AddObserver(&observer);

  pref_service_.SetBoolean(kReferralCheckedForPromoCodeFile, true);
  pref_service_.SetBoolean(kReferralInitialization, true);

  // Check with json file w/o schema version with empty object.
  service_->si_images_data_.reset();
  service_->OnGetSponsoredComponentJsonData(false, "{}");
  EXPECT_EQ(nullptr, service_->GetBrandedImagesData(false));
  service_->bi_images_data_.reset();
  service_->OnGetComponentJsonData("{}");
  EXPECT_EQ(nullptr, service_->GetBackgroundImagesData());

  // Check with json file with empty object.
  service_->si_images_data_.reset();
  observer.on_si_updated_ = false;
  observer.si_data_ = nullptr;
  service_->OnGetSponsoredComponentJsonData(false, kTestEmptyComponent);
  auto* si_data = service_->GetBrandedImagesData(false);
  EXPECT_EQ(si_data, nullptr);
  EXPECT_TRUE(observer.on_si_updated_);
  EXPECT_TRUE(observer.si_data_->campaigns.empty());
  service_->bi_images_data_.reset();
  observer.on_bi_updated_ = false;
  observer.bi_data_ = nullptr;
  service_->OnGetComponentJsonData(kTestEmptyComponent);
  auto* bi_data = service_->GetBackgroundImagesData();
  EXPECT_EQ(bi_data, nullptr);
  EXPECT_TRUE(observer.on_bi_updated_);
  EXPECT_FALSE(observer.bi_data_->IsValid());

  service_->si_images_data_.reset();
  observer.on_si_updated_ = false;
  observer.si_data_ = nullptr;
  service_->OnGetSponsoredComponentJsonData(false, kTestSponsoredImages);
  // Mark this is not SR to get SI data.
  service_->MarkThisInstallIsNotSuperReferralForever();
  si_data = service_->GetBrandedImagesData(false);
  EXPECT_TRUE(si_data);
  EXPECT_TRUE(si_data->IsValid());
  EXPECT_FALSE(si_data->IsSuperReferral());
  // Above json data has 3 wallpapers.
  EXPECT_EQ(1UL, si_data->campaigns.size());
  const auto campaign = si_data->campaigns[0];
  const size_t image_count = 3;
  EXPECT_EQ(image_count, campaign.backgrounds.size());
  EXPECT_EQ(696, campaign.backgrounds[0].focal_point.x());
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("background-1.jpg"),
            campaign.backgrounds[0].image_file.BaseName());
  // Check default value is set if "focalPoint" is missed.
  EXPECT_EQ(0, campaign.backgrounds[1].focal_point.x());
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("background-2.jpg"),
            campaign.backgrounds[1].image_file.BaseName());
  EXPECT_EQ(0, campaign.backgrounds[2].focal_point.x());
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("background-3.jpg"),
            campaign.backgrounds[2].image_file.BaseName());
  EXPECT_TRUE(observer.on_si_updated_);
  EXPECT_FALSE(
      observer.si_data_->campaigns[0].backgrounds[0].logo.alt_text.empty());
  EXPECT_TRUE(*si_data->GetBackgroundAt(0, 0).FindBoolKey(kIsSponsoredKey));
  EXPECT_FALSE(*si_data->GetBackgroundAt(0, 0).FindBoolKey(kIsBackgroundKey));

  // Default logo is used for wallpaper at 0.
  EXPECT_EQ("logo.png",
            *si_data->GetBackgroundAt(0, 0).FindStringPath(kLogoImagePath));
  // Per wallpaper logo is used for wallpaper at 1.
  EXPECT_EQ("logo-2.png",
            *si_data->GetBackgroundAt(0, 1).FindStringPath(kLogoImagePath));

  // Test BI data loading
  service_->bi_images_data_.reset();
  observer.on_bi_updated_ = false;
  observer.bi_data_ = nullptr;
  service_->OnGetComponentJsonData(kTestBackgroundImages);
  bi_data = service_->GetBackgroundImagesData();
  EXPECT_TRUE(bi_data);
  EXPECT_TRUE(bi_data->IsValid());
  // Above json data has 2 wallpapers.
  const size_t bi_image_count = 2;
  EXPECT_EQ(bi_image_count, bi_data->backgrounds.size());
  // Check values are loaded correctly
  EXPECT_EQ("Brave Software", bi_data->backgrounds[0].author);
  EXPECT_EQ("https://brave.com/", bi_data->backgrounds[0].link);
  EXPECT_TRUE(observer.on_bi_updated_);
  EXPECT_TRUE(*bi_data->GetBackgroundAt(0).FindBoolKey(kIsBackgroundKey));
  EXPECT_EQ("chrome://background-wallpaper/background-image-source.webp",
            *bi_data->GetBackgroundAt(0).FindStringKey(kWallpaperImageURLKey));
  EXPECT_EQ("background-image-source.webp",
            *bi_data->GetBackgroundAt(0).FindStringKey(kWallpaperImagePathKey));
  EXPECT_EQ("chrome://background-wallpaper/background-image-source.avif",
            *bi_data->GetBackgroundAt(1).FindStringKey(kWallpaperImageURLKey));
  EXPECT_EQ("background-image-source.avif",
            *bi_data->GetBackgroundAt(1).FindStringKey(kWallpaperImagePathKey));

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
  observer.on_si_updated_ = false;
  observer.si_data_ = nullptr;
  service_->OnGetSponsoredComponentJsonData(false,
                                            test_json_string_higher_schema);
  si_data = service_->GetBrandedImagesData(false);
  EXPECT_FALSE(si_data);

  constexpr char test_background_json_string_higher_schema[] = R"(
  {
    "schemaVersion": 2,
    "images": [
      {
        "name": "ntp-2020/2021-1",
        "source": "background-image-source.png",
        "author": "Brave Software",
        "link": "https://brave.com/",
        "originalUrl": "Contributor sent the hi-res version through email",
        "license": "https://brave.com/about/"
      }
    ]
  })";
  service_->bi_images_data_.reset();
  observer.on_bi_updated_ = false;
  observer.bi_data_ = nullptr;
  service_->OnGetComponentJsonData(test_background_json_string_higher_schema);
  bi_data = service_->GetBackgroundImagesData();
  EXPECT_FALSE(bi_data);

  service_->RemoveObserver(&observer);
}

TEST_F(NTPBackgroundImagesServiceTest, MultipleCampaignsTest) {
  Init();
  TestObserver observer;
  service_->AddObserver(&observer);

  pref_service_.SetBoolean(kReferralCheckedForPromoCodeFile, true);
  pref_service_.SetBoolean(kReferralInitialization, true);

  service_->si_images_data_.reset();
  observer.on_si_updated_ = false;
  observer.si_data_ = nullptr;
  service_->OnGetSponsoredComponentJsonData(
      false, kTestSponsoredImagesWithMultipleCampaigns);
  // Mark this is not SR to get SI data.
  service_->MarkThisInstallIsNotSuperReferralForever();
  auto* si_data = service_->GetBrandedImagesData(false);
  EXPECT_TRUE(si_data);
  EXPECT_TRUE(si_data->IsValid());
  EXPECT_FALSE(si_data->IsSuperReferral());
  EXPECT_EQ(2UL, si_data->campaigns.size());
  const auto campaign_0 = si_data->campaigns[0];
  EXPECT_EQ(3UL, campaign_0.backgrounds.size());
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("background-1.jpg"),
            campaign_0.backgrounds[0].image_file.BaseName());
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("logo.png"),
            campaign_0.backgrounds[0].logo.image_file.BaseName());
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("logo-2.png"),
            campaign_0.backgrounds[1].logo.image_file.BaseName());

  const auto campaign_1 = si_data->campaigns[1];
  EXPECT_EQ(2UL, campaign_1.backgrounds.size());
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("background-4.jpg"),
            campaign_1.backgrounds[0].image_file.BaseName());
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("background-5.jpg"),
            campaign_1.backgrounds[1].image_file.BaseName());
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("logo-4.png"),
            campaign_1.backgrounds[1].logo.image_file.BaseName());

  service_->RemoveObserver(&observer);
}

#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)

#if defined(OS_LINUX)

// Linux doesn't support referral service now.
// So, always start NTP SI component.
TEST_F(NTPBackgroundImagesServiceTest, TestOnNonReferralService) {
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
  observer.on_si_updated_ = false;
  observer.si_data_ = nullptr;
  service_->OnGetSponsoredComponentJsonData(true, kTestSuperReferral);
  auto* data = service_->GetBrandedImagesData(true);
  EXPECT_TRUE(data);

  const size_t wallpaper_count = 3;
  const size_t top_site_count = 3;
  EXPECT_EQ(wallpaper_count, data->campaigns[0].backgrounds.size());
  EXPECT_EQ(top_site_count, data->top_sites.size());
  EXPECT_TRUE(data->IsSuperReferral());
  EXPECT_FALSE(*data->GetBackgroundAt(0, 0).FindBoolKey(kIsSponsoredKey));
  EXPECT_TRUE(observer.on_si_updated_);

  service_->RemoveObserver(&observer);
}

// Test default referral code and first run.
// Sponsored Images component will be run after promo code set to pref.
TEST_F(NTPBackgroundImagesServiceTest, WithDefaultReferralCodeTest1) {
  Init();
  TestObserver observer;
  service_->AddObserver(&observer);

  // Initially, only SI is started and pref is monitored to get referral code.
  EXPECT_TRUE(service_->sponsored_images_component_started_);
  EXPECT_TRUE(service_->referral_promo_code_change_monitored_);
  EXPECT_TRUE(service_->checked_super_referral_component_);
  EXPECT_FALSE(service_->mapping_table_requested_);
  EXPECT_FALSE(service_->super_referral_component_started_);
  EXPECT_FALSE(service_->marked_this_install_is_not_super_referral_forever_);

  observer.on_super_referral_ended_ = false;
  pref_service_.SetString(kReferralPromoCode, "BRV001");
  EXPECT_TRUE(service_->marked_this_install_is_not_super_referral_forever_);
  // We should notify OnSuperReferralEnded() if this is not NTP SR
  // (default promo code).
  EXPECT_TRUE(observer.on_super_referral_ended_);
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
  TestObserver observer;
  service_->AddObserver(&observer);

  EXPECT_TRUE(service_->sponsored_images_component_started_);
  EXPECT_TRUE(service_->checked_super_referral_component_);
  EXPECT_TRUE(service_->referral_promo_code_change_monitored_);
  EXPECT_FALSE(service_->mapping_table_requested_);
  EXPECT_FALSE(service_->super_referral_component_started_);

  pref_service_.SetString(kReferralPromoCode, "BRV002");

  // Mapping table is requested because it's not a default code.
  EXPECT_TRUE(service_->mapping_table_requested_);
  EXPECT_FALSE(service_->marked_this_install_is_not_super_referral_forever_);

  // Initialize NTP SI data.
  service_->OnGetSponsoredComponentJsonData(false, kTestSponsoredImages);
  // NTP SI data is ready but don't give data until NTP SR initialization is
  // complete. Only gives NTP SI data when browser confirms this is not NTP SR.
  EXPECT_EQ(nullptr, service_->GetBrandedImagesData(false));

  observer.on_super_referral_ended_ = false;
  service_->OnGetMappingTableData(kTestMappingTable);
  // We should notify OnSuperReferralEnded() if this is not NTP SR.
  EXPECT_TRUE(observer.on_super_referral_ended_);

  // If it's not super-referral, we mark this install is not a valid SR.
  EXPECT_TRUE(service_->marked_this_install_is_not_super_referral_forever_);
  EXPECT_FALSE(service_->super_referral_component_started_);
}

TEST_F(NTPBackgroundImagesServiceTest, WithSuperReferralCodeTest) {
  EXPECT_FALSE(pref_service_.GetBoolean(
      prefs::kNewTabPageGetInitialSRComponentInProgress));

  Init();
  TestObserver observer;
  service_->AddObserver(&observer);

  EXPECT_TRUE(service_->sponsored_images_component_started_);
  EXPECT_TRUE(service_->checked_super_referral_component_);
  EXPECT_TRUE(service_->referral_promo_code_change_monitored_);
  EXPECT_FALSE(service_->mapping_table_requested_);
  EXPECT_FALSE(service_->super_referral_component_started_);

  EXPECT_TRUE(pref_service_.GetString(
      prefs::kNewTabPageCachedSuperReferralCode).empty());
  EXPECT_TRUE(pref_service_.GetBoolean(
      prefs::kNewTabPageGetInitialSRComponentInProgress));
  pref_service_.SetString(kReferralPromoCode, "BRV003");

  // Mapping table is requested because it's not a default code.
  EXPECT_TRUE(service_->mapping_table_requested_);
  EXPECT_FALSE(service_->marked_this_install_is_not_super_referral_forever_);

  EXPECT_FALSE(service_->IsValidSuperReferralComponentInfo(*pref_service_.Get(
      prefs::kNewTabPageCachedSuperReferralComponentInfo)));
  service_->OnGetMappingTableData(kTestMappingTable);
  EXPECT_TRUE(pref_service_.GetBoolean(
      prefs::kNewTabPageGetInitialSRComponentInProgress));
  EXPECT_EQ("BRV003",
            pref_service_.GetString(prefs::kNewTabPageCachedSuperReferralCode));
  // This is super referral code. So, start SR component.
  EXPECT_TRUE(service_->super_referral_component_started_);
  EXPECT_FALSE(service_->marked_this_install_is_not_super_referral_forever_);

  EXPECT_TRUE(pref_service_.GetString(
                  prefs::kNewTabPageCachedSuperReferralComponentData).empty());

  // Got super referral component
  service_->OnGetSponsoredComponentJsonData(true, kTestSuperReferral);
  EXPECT_FALSE(pref_service_.GetBoolean(
      prefs::kNewTabPageGetInitialSRComponentInProgress));
  auto* data = service_->GetBrandedImagesData(true);
  EXPECT_TRUE(service_->IsValidSuperReferralComponentInfo(*pref_service_.Get(
      prefs::kNewTabPageCachedSuperReferralComponentInfo)));
  EXPECT_TRUE(data->IsSuperReferral());
  EXPECT_FALSE(pref_service_.GetString(
                   prefs::kNewTabPageCachedSuperReferralComponentData).empty());

  // Simulate current SR campaign is ended.
  service_->OnGetSponsoredComponentJsonData(true, kTestEmptyComponent);
  EXPECT_TRUE(observer.on_super_referral_ended_);
  EXPECT_TRUE(pref_service_.GetString(
      prefs::kNewTabPageCachedSuperReferralCode).empty());
  EXPECT_FALSE(service_->IsValidSuperReferralComponentInfo(*pref_service_.Get(
      prefs::kNewTabPageCachedSuperReferralComponentInfo)));
  EXPECT_TRUE(pref_service_.GetString(
                  prefs::kNewTabPageCachedSuperReferralComponentData).empty());
  EXPECT_TRUE(service_->marked_this_install_is_not_super_referral_forever_);
  EXPECT_TRUE(service_->unregistered_super_referral_component_);
  service_->RemoveObserver(&observer);
}

TEST_F(NTPBackgroundImagesServiceTest, CheckReferralServiceInitStatusTest) {
  Init();

  // Initially, data is not available.
  auto* data = service_->GetBrandedImagesData(true);
  EXPECT_FALSE(data);
  data = service_->GetBrandedImagesData(false);
  EXPECT_FALSE(data);

  // Simulate SI data is initialized first before referral service is
  // initialized.
  // Check SI data is not available before referrals service is initialized.
  service_->OnGetSponsoredComponentJsonData(false, kTestSponsoredImages);
  data = service_->GetBrandedImagesData(false);
  EXPECT_FALSE(data);

  // Simulate that this install is not SR. Then, SI data is returned properly.
  service_->MarkThisInstallIsNotSuperReferralForever();
  data = service_->GetBrandedImagesData(false);
  EXPECT_TRUE(data);
}

TEST_F(NTPBackgroundImagesServiceTest,
       CheckRecoverShutdownWhileMappingTableFetchingWithDefaultCode) {
  // Make this install has initialized super referral service.
  pref_service_.SetBoolean(kReferralCheckedForPromoCodeFile, true);
  pref_service_.SetBoolean(kReferralInitialization, true);
  pref_service_.SetBoolean(
      prefs::kNewTabPageGetInitialSRComponentInProgress, true);
  pref_service_.SetString(kReferralPromoCode, "BRV001");

  EXPECT_TRUE(pref_service_.FindPreference(
      prefs::kNewTabPageCachedSuperReferralComponentInfo)->IsDefaultValue());

  Init();

  EXPECT_FALSE(pref_service_.FindPreference(
      prefs::kNewTabPageCachedSuperReferralComponentInfo)->IsDefaultValue());
  // In this case, directly request mapping table w/o monitoring promoCode pref
  // changing.
  EXPECT_FALSE(service_->mapping_table_requested_);
  EXPECT_FALSE(service_->referral_promo_code_change_monitored_);
}

TEST_F(NTPBackgroundImagesServiceTest,
       CheckRecoverShutdownWhileMappingTableFetchingWithNonDefaultCode) {
  // Make this install has initialized super referral service.
  pref_service_.SetBoolean(kReferralCheckedForPromoCodeFile, true);
  pref_service_.SetBoolean(kReferralInitialization, true);
  pref_service_.SetBoolean(
      prefs::kNewTabPageGetInitialSRComponentInProgress, true);
  pref_service_.SetString(kReferralPromoCode, "BRV003");

  Init();

  // In this case, directly request mapping table w/o monitoring promoCode pref
  // changing.
  EXPECT_TRUE(service_->mapping_table_requested_);
  EXPECT_FALSE(service_->referral_promo_code_change_monitored_);
}

#endif  // OS_LINUX

#endif  // BUILDFLAG(ENABLE_BRAVE_REFERRALS)

}  // namespace ntp_background_images
