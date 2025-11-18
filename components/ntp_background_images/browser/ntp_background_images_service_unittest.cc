/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/test/task_environment.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "build/build_config.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

constexpr char kTestEmptyComponent[] = R"(
    {
        "schemaVersion": 2
    })";

constexpr char kTestSponsoredImages[] = R"(
    {
      "schemaVersion": 2,
      "campaigns": [
        {
          "version": 1,
          "campaignId": "65933e82-6b21-440b-9956-c0f675ca7435",
          "creativeSets": [
            {
              "creativeSetId": "6690ad47-d0af-4dbb-a2dd-c7a678b2b83b",
              "creatives": [
                {
                  "creativeInstanceId": "30244a36-561a-48f0-8d7a-780e9035c57a",
                  "companyName": "Image NTT Creative",
                  "alt": "Some content",
                  "targetUrl": "https://basicattentiontoken.org",
                  "wallpaper": {
                    "type": "image",
                    "relativeUrl": "30244a36-561a-48f0-8d7a-780e9035c57a/background.jpg",
                    "focalPoint": {
                      "x": 25,
                      "y": 50
                    },
                    "button": {
                      "image": {
                        "relativeUrl": "30244a36-561a-48f0-8d7a-780e9035c57a/button.png"
                      }
                    }
                  }
                }
              ]
            }
          ]
        }
      ]
    })";

constexpr char kTestSponsoredImagesWithMultipleCampaigns[] = R"(
    {
      "schemaVersion": 2,
      "campaigns": [
        {
          "version": 1,
          "campaignId": "65933e82-6b21-440b-9956-c0f675ca7435",
          "creativeSets": [
            {
              "creativeSetId": "6690ad47-d0af-4dbb-a2dd-c7a678b2b83b",
              "creatives": [
                {
                  "creativeInstanceId": "30244a36-561a-48f0-8d7a-780e9035c57a",
                  "companyName": "Image NTT Creative",
                  "alt": "Some content",
                  "targetUrl": "https://basicattentiontoken.org",
                  "wallpaper": {
                    "type": "image",
                    "relativeUrl": "30244a36-561a-48f0-8d7a-780e9035c57a/background-1.jpg",
                    "focalPoint": {
                      "x": 25,
                      "y": 50
                    },
                    "button": {
                      "image": {
                        "relativeUrl": "30244a36-561a-48f0-8d7a-780e9035c57a/button-1.png"
                      }
                    }
                  }
                }
              ]
            }
          ]
        },
        {
          "version": 1,
          "campaignId": "c27a3fae-ee9e-48a2-b3a7-f4675744e6ec",
          "creativeSets": [
            {
              "creativeSetId": "a245e3b9-2df4-47f5-aaab-67b61c528b6f",
              "creatives": [
                {
                  "creativeInstanceId": "39d78863-327d-4b64-9952-cd0e5e330eb6",
                  "alt": "Some more rich content",
                  "companyName": "Another Rich Media NTT Creative",
                  "targetUrl": "https://basicattentiontoken.org",
                  "wallpaper": {
                    "type": "richMedia",
                    "relativeUrl": "39d78863-327d-4b64-9952-cd0e5e330eb6/index.html"
                  }
                }
              ]
            }
          ]
        }
      ]
    })";

constexpr char kTestSponsoredImagesWithMissingImageUrl[] = R"(
    {
      "schemaVersion": 2,
      "campaigns": [
        {
          "version": 1,
          "campaignId": "65933e82-6b21-440b-9956-c0f675ca7435",
          "creativeSets": [
            {
              "creativeSetId": "6690ad47-d0af-4dbb-a2dd-c7a678b2b83b",
              "creatives": [
                {
                  "creativeInstanceId": "30244a36-561a-48f0-8d7a-780e9035c57a",
                  "companyName": "Image NTT Creative",
                  "alt": "Some content",
                  "targetUrl": "https://basicattentiontoken.org",
                  "wallpaper": {
                    "type": "image",
                    "missing_relativeUrl": "30244a36-561a-48f0-8d7a-780e9035c57a/background.jpg",
                    "focalPoint": {
                      "x": 25,
                      "y": 50
                    },
                    "button": {
                      "image": {
                        "relativeUrl": "30244a36-561a-48f0-8d7a-780e9035c57a/button.png"
                      }
                    }
                  }
                }
              ]
            }
          ]
        }
      ]
    })";

constexpr char kSponsoredImageContentWithNonHttpsSchemeTargetUrl[] = R"(
    {
      "schemaVersion": 2,
      "campaigns": [
        {
          "version": 1,
          "campaignId": "65933e82-6b21-440b-9956-c0f675ca7435",
          "creativeSets": [
            {
              "creativeSetId": "6690ad47-d0af-4dbb-a2dd-c7a678b2b83b",
              "creatives": [
                {
                  "creativeInstanceId": "30244a36-561a-48f0-8d7a-780e9035c57a",
                  "companyName": "Image NTT Creative",
                  "alt": "Some content",
                  "targetUrl": "http://basicattentiontoken.org",
                  "wallpaper": {
                    "type": "image",
                    "missing_relativeUrl": "30244a36-561a-48f0-8d7a-780e9035c57a/background.jpg",
                    "focalPoint": {
                      "x": 25,
                      "y": 50
                    },
                    "button": {
                      "image": {
                        "relativeUrl": "30244a36-561a-48f0-8d7a-780e9035c57a/button.png"
                      }
                    }
                  }
                }
              ]
            }
          ]
        }
      ]
    })";

constexpr char
    kSponsoredImageContentWithWallpaperRelativeUrlReferencingParent[] = R"(
    {
      "schemaVersion": 2,
      "campaigns": [
        {
          "version": 1,
          "campaignId": "65933e82-6b21-440b-9956-c0f675ca7435",
          "creativeSets": [
            {
              "creativeSetId": "6690ad47-d0af-4dbb-a2dd-c7a678b2b83b",
              "creatives": [
                {
                  "creativeInstanceId": "30244a36-561a-48f0-8d7a-780e9035c57a",
                  "companyName": "Image NTT Creative",
                  "alt": "Some content",
                  "targetUrl": "https://basicattentiontoken.org",
                  "wallpaper": {
                    "type": "image",
                    "relativeUrl": "../background.jpg",
                    "focalPoint": {
                      "x": 25,
                      "y": 50
                    },
                    "button": {
                      "image": {
                        "relativeUrl": "30244a36-561a-48f0-8d7a-780e9035c57a/button.png"
                      }
                    }
                  }
                }
              ]
            }
          ]
        }
      ]
    })";

constexpr char
    kSponsoredImageContentWithWallpaperButtonImageRelativeUrlReferencingParent
        [] = R"(
    {
      "schemaVersion": 2,
      "campaigns": [
        {
          "version": 1,
          "campaignId": "65933e82-6b21-440b-9956-c0f675ca7435",
          "creativeSets": [
            {
              "creativeSetId": "6690ad47-d0af-4dbb-a2dd-c7a678b2b83b",
              "creatives": [
                {
                  "creativeInstanceId": "30244a36-561a-48f0-8d7a-780e9035c57a",
                  "companyName": "Image NTT Creative",
                  "alt": "Some content",
                  "targetUrl": "https://basicattentiontoken.org",
                  "wallpaper": {
                    "type": "image",
                    "relativeUrl": "30244a36-561a-48f0-8d7a-780e9035c57a/background.jpg",
                    "focalPoint": {
                      "x": 25,
                      "y": 50
                    },
                    "button": {
                      "image": {
                        "relativeUrl": "../button.png"
                      }
                    }
                  }
                }
              ]
            }
          ]
        }
      ]
    })";

constexpr char
    kSponsoredRichMediaContentWithWallpaperRelativeUrlReferencingParent[] = R"(
        {
          "schemaVersion": 2,
          "campaigns": [
            {
              "version": 1,
              "campaignId": "65933e82-6b21-440b-9956-c0f675ca7435",
              "creativeSets": [
                {
                  "creativeSetId": "6690ad47-d0af-4dbb-a2dd-c7a678b2b83b",
                  "creatives": [
                    {
                      "creativeInstanceId": "39d78863-327d-4b64-9952-cd0e5e330eb6",
                      "alt": "Some more rich content",
                      "companyName": "Another Rich Media NTT Creative",
                      "targetUrl": "https://basicattentiontoken.org",
                      "wallpaper": {
                        "type": "richMedia",
                        "relativeUrl": "../index.html"
                      }
                    }
                  ]
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

class ObserverMock : public NTPBackgroundImagesService::Observer {
 public:
  ObserverMock() = default;
  ~ObserverMock() override = default;

  void OnBackgroundImagesDataDidUpdate(NTPBackgroundImagesData* data) override {
    on_background_images_updated = true;
    background_images_data = data;
  }

  void OnSponsoredImagesDataDidUpdate(NTPSponsoredImagesData* data) override {
    on_sponsored_images_updated = true;
    sponsored_images_data = data;
  }

  raw_ptr<NTPBackgroundImagesData> background_images_data = nullptr;
  bool on_background_images_updated = false;

  raw_ptr<NTPSponsoredImagesData, DanglingUntriaged> sponsored_images_data =
      nullptr;
  bool on_sponsored_images_updated = false;
};

class NTPBackgroundImagesServiceForTesting : public NTPBackgroundImagesService {
 public:
  using NTPBackgroundImagesService::NTPBackgroundImagesService;

  void RegisterSponsoredImagesComponent() override {
    NTPBackgroundImagesService::RegisterSponsoredImagesComponent();
    sponsored_images_component_started = true;
  }

  void RegisterBackgroundImagesComponent() override {
    NTPBackgroundImagesService::RegisterBackgroundImagesComponent();
    background_images_component_started = true;
  }

  bool sponsored_images_component_started = false;
  bool background_images_component_started = false;
  bool mapping_table_requested = false;
  bool referral_promo_code_change_monitored = false;
};

class NTPBackgroundImagesServiceTest : public testing::Test {
 public:
  NTPBackgroundImagesServiceTest() = default;

  void SetUp() override {
    PrefRegistrySimple* const pref_registry = pref_service_.registry();
    NTPBackgroundImagesService::RegisterLocalStatePrefsForMigration(
        pref_registry);
  }

  void TearDown() override {
    if (service_) {
      service_->RemoveObserver(&observer_);
    }
  }

  void Init() {
    service_ = std::make_unique<NTPBackgroundImagesServiceForTesting>(
        /*variations_service=*/nullptr, /*component_update_service=*/nullptr,
        &pref_service_);
    service_->Init();
    service_->AddObserver(&observer_);
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<NTPBackgroundImagesServiceForTesting> service_;
  ObserverMock observer_;
};

TEST_F(NTPBackgroundImagesServiceTest, BasicTest) {
  Init();
  // NTP SI Component is registered after ads is initialized.
  EXPECT_FALSE(service_->sponsored_images_component_started);
  // If ENABLE_NTP_BACKGROUND_IMAGES then BI shall be registered
  EXPECT_TRUE(service_->background_images_component_started);
}

TEST_F(NTPBackgroundImagesServiceTest, InternalDataTest) {
  Init();

  // Check with json file w/o schema version with empty object.
  service_->sponsored_images_data_.reset();
  service_->OnGetSponsoredComponentJsonData("{}");
  EXPECT_FALSE(service_->GetSponsoredImagesData(
      /*supports_rich_media=*/true));
  service_->background_images_data_.reset();
  service_->OnGetComponentJsonData("{}");
  EXPECT_FALSE(service_->GetBackgroundImagesData());

  // Check with json file with empty object.
  observer_.sponsored_images_data = nullptr;
  service_->sponsored_images_data_.reset();
  observer_.on_sponsored_images_updated = false;
  service_->OnGetSponsoredComponentJsonData(kTestEmptyComponent);
  EXPECT_FALSE(service_->GetSponsoredImagesData(/*supports_rich_media=*/true));
  EXPECT_TRUE(observer_.on_sponsored_images_updated);
  EXPECT_THAT(observer_.sponsored_images_data->campaigns, ::testing::IsEmpty());
  observer_.background_images_data = nullptr;
  service_->background_images_data_.reset();
  observer_.on_background_images_updated = false;
  service_->OnGetComponentJsonData(kTestEmptyComponent);
  NTPBackgroundImagesData* background_images_data =
      service_->GetBackgroundImagesData();
  EXPECT_FALSE(background_images_data);
  EXPECT_TRUE(observer_.on_background_images_updated);
  EXPECT_FALSE(observer_.background_images_data->IsValid());

  observer_.sponsored_images_data = nullptr;
  service_->sponsored_images_data_.reset();
  observer_.on_sponsored_images_updated = false;
  service_->OnGetSponsoredComponentJsonData(kTestSponsoredImages);
  NTPSponsoredImagesData* const images_data =
      service_->GetSponsoredImagesData(/*supports_rich_media=*/true);
  EXPECT_TRUE(images_data);
  EXPECT_TRUE(images_data->IsValid());
  // Above json data has 3 wallpapers.
  EXPECT_THAT(images_data->campaigns, ::testing::SizeIs(1));
  const Campaign campaign = images_data->campaigns[0];
  EXPECT_THAT(campaign.campaign_id, ::testing::Not(::testing::IsEmpty()));
  EXPECT_THAT(campaign.creatives, ::testing::SizeIs(1));
  EXPECT_EQ(25, campaign.creatives[0].focal_point.x());
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("background.jpg"),
            campaign.creatives[0].file_path.BaseName());
  EXPECT_EQ(campaign.creatives[0].creative_instance_id,
            "30244a36-561a-48f0-8d7a-780e9035c57a");
  EXPECT_TRUE(observer_.on_sponsored_images_updated);
  EXPECT_THAT(
      observer_.sponsored_images_data->campaigns[0].creatives[0].logo.alt_text,
      ::testing::Not(::testing::IsEmpty()));
  EXPECT_TRUE(
      images_data->MaybeGetBackgroundAt(0, 0)->FindBool(kIsSponsoredKey));
  EXPECT_FALSE(images_data->MaybeGetBackgroundAt(0, 0)
                   ->FindBool(kIsBackgroundKey)
                   .value());

  EXPECT_EQ("30244a36-561a-48f0-8d7a-780e9035c57a/button.png",
            *images_data->MaybeGetBackgroundAt(0, 0)->FindStringByDottedPath(
                kLogoImagePath));

  // Test BI data loading
  observer_.background_images_data = nullptr;
  service_->background_images_data_.reset();
  observer_.on_background_images_updated = false;
  service_->OnGetComponentJsonData(kTestBackgroundImages);
  background_images_data = service_->GetBackgroundImagesData();
  EXPECT_TRUE(background_images_data);
  EXPECT_TRUE(background_images_data->IsValid());
  // Above json data has 2 wallpapers.
  EXPECT_THAT(background_images_data->backgrounds, ::testing::SizeIs(2));
  // Check values are loaded correctly
  EXPECT_EQ("Brave Software", background_images_data->backgrounds[0].author);
  EXPECT_EQ("https://brave.com/", background_images_data->backgrounds[0].link);
  EXPECT_TRUE(observer_.on_background_images_updated);
  EXPECT_TRUE(
      *background_images_data->GetBackgroundAt(0).FindBool(kIsBackgroundKey));
  EXPECT_EQ(
      "chrome://background-wallpaper/background-image-source.webp",
      *background_images_data->GetBackgroundAt(0).FindString(kWallpaperURLKey));
  EXPECT_EQ("background-image-source.webp",
            *background_images_data->GetBackgroundAt(0).FindString(
                kWallpaperFilePathKey));
  EXPECT_EQ(
      "chrome://background-wallpaper/background-image-source.avif",
      *background_images_data->GetBackgroundAt(1).FindString(kWallpaperURLKey));
  EXPECT_EQ("background-image-source.avif",
            *background_images_data->GetBackgroundAt(1).FindString(
                kWallpaperFilePathKey));

  // Invalid schema version
  const std::string test_json_string_higher_schema = R"(
    {
      "schemaVersion": -1,
      "campaigns": [
        {
          "version": 1,
          "campaignId": "65933e82-6b21-440b-9956-c0f675ca7435",
          "creativeSets": [
            {
              "creativeSetId": "6690ad47-d0af-4dbb-a2dd-c7a678b2b83b",
              "creatives": [
                {
                  "creativeInstanceId": "30244a36-561a-48f0-8d7a-780e9035c57a",
                  "companyName": "Image NTT Creative",
                  "alt": "Some content",
                  "targetUrl": "https://basicattentiontoken.org",
                  "wallpaper": {
                    "type": "image",
                    "relativeUrl": "30244a36-561a-48f0-8d7a-780e9035c57a/background.jpg",
                    "focalPoint": {
                      "x": 25,
                      "y": 50
                    },
                    "button": {
                      "image": {
                        "relativeUrl": "30244a36-561a-48f0-8d7a-780e9035c57a/button.png"
                      }
                    }
                  }
                }
              ]
            }
          ]
        }
      ]
    })";
  observer_.sponsored_images_data = nullptr;
  service_->sponsored_images_data_.reset();
  observer_.on_sponsored_images_updated = false;
  service_->OnGetSponsoredComponentJsonData(test_json_string_higher_schema);
  EXPECT_FALSE(service_->GetSponsoredImagesData(/*supports_rich_media=*/true));

  constexpr char kTestBackgroundJsonStringHigherSchema[] = R"(
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
  observer_.background_images_data = nullptr;
  service_->background_images_data_.reset();
  observer_.on_background_images_updated = false;
  service_->OnGetComponentJsonData(kTestBackgroundJsonStringHigherSchema);
  background_images_data = service_->GetBackgroundImagesData();
  EXPECT_FALSE(background_images_data);
}

TEST_F(NTPBackgroundImagesServiceTest, MultipleCampaignsTest) {
  Init();

  observer_.sponsored_images_data = nullptr;
  service_->sponsored_images_data_.reset();
  observer_.on_sponsored_images_updated = false;
  service_->OnGetSponsoredComponentJsonData(
      kTestSponsoredImagesWithMultipleCampaigns);
  const NTPSponsoredImagesData* const images_data =
      service_->GetSponsoredImagesData(/*supports_rich_media=*/true);
  EXPECT_TRUE(images_data);
  EXPECT_TRUE(images_data->IsValid());
  EXPECT_THAT(images_data->campaigns, ::testing::SizeIs(2));
  const Campaign campaign_0 = images_data->campaigns[0];
  EXPECT_THAT(campaign_0.campaign_id, ::testing::Not(::testing::IsEmpty()));
  EXPECT_THAT(campaign_0.creatives, ::testing::SizeIs(1));
  EXPECT_THAT(campaign_0.creatives[0].creative_instance_id,
              ::testing::Not(::testing::IsEmpty()));
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("background-1.jpg"),
            campaign_0.creatives[0].file_path.BaseName());
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("button-1.png"),
            campaign_0.creatives[0].logo.image_file.BaseName());

  const Campaign campaign_1 = images_data->campaigns[1];
  EXPECT_THAT(campaign_1.campaign_id, ::testing::Not(::testing::IsEmpty()));
  EXPECT_THAT(campaign_1.creatives, ::testing::SizeIs(1));
  EXPECT_THAT(campaign_1.creatives[0].creative_instance_id,
              ::testing::Not(::testing::IsEmpty()));
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("index.html"),
            campaign_1.creatives[0].file_path.BaseName());
}

TEST_F(NTPBackgroundImagesServiceTest,
       DoNotGetSponsoredImageContentForNonHttpsSchemeTargetUrl) {
  Init();

  observer_.sponsored_images_data = nullptr;
  service_->sponsored_images_data_.reset();
  observer_.on_sponsored_images_updated = false;
  service_->OnGetSponsoredComponentJsonData(
      kSponsoredImageContentWithNonHttpsSchemeTargetUrl);

  EXPECT_FALSE(service_->GetSponsoredImagesData(/*supports_rich_media=*/true));
  EXPECT_TRUE(observer_.on_sponsored_images_updated);
  EXPECT_THAT(observer_.sponsored_images_data->campaigns, ::testing::IsEmpty());
  EXPECT_THAT(service_->sponsored_images_data_->campaigns,
              ::testing::IsEmpty());
}

TEST_F(NTPBackgroundImagesServiceTest,
       DoNotGetSponsoredImageContentIfWallpaperUrlReferencesParent) {
  Init();

  observer_.sponsored_images_data = nullptr;
  service_->sponsored_images_data_.reset();
  observer_.on_sponsored_images_updated = false;
  service_->OnGetSponsoredComponentJsonData(
      kSponsoredImageContentWithWallpaperRelativeUrlReferencingParent);

  EXPECT_FALSE(service_->GetSponsoredImagesData(/*supports_rich_media=*/true));
  EXPECT_TRUE(observer_.on_sponsored_images_updated);
  EXPECT_THAT(observer_.sponsored_images_data->campaigns, ::testing::IsEmpty());
  EXPECT_THAT(service_->sponsored_images_data_->campaigns,
              ::testing::IsEmpty());
}

TEST_F(
    NTPBackgroundImagesServiceTest,
    DoNotGetSponsoredImageContentIfWallpaperButtonImageRelativeUrlReferencesParent) {
  Init();

  observer_.sponsored_images_data = nullptr;
  service_->sponsored_images_data_.reset();
  observer_.on_sponsored_images_updated = false;
  service_->OnGetSponsoredComponentJsonData(
      kSponsoredImageContentWithWallpaperButtonImageRelativeUrlReferencingParent);

  EXPECT_FALSE(service_->GetSponsoredImagesData(/*supports_rich_media=*/true));
  EXPECT_TRUE(observer_.on_sponsored_images_updated);
  EXPECT_THAT(observer_.sponsored_images_data->campaigns, ::testing::IsEmpty());
  EXPECT_THAT(service_->sponsored_images_data_->campaigns,
              ::testing::IsEmpty());
}

TEST_F(
    NTPBackgroundImagesServiceTest,
    DoNotGetSponsoredRichMediaContentIfWallpaperRelativeUrlReferencesParent) {
  Init();

  observer_.sponsored_images_data = nullptr;
  service_->sponsored_images_data_.reset();
  observer_.on_sponsored_images_updated = false;
  service_->OnGetSponsoredComponentJsonData(
      kSponsoredRichMediaContentWithWallpaperRelativeUrlReferencingParent);

  EXPECT_FALSE(service_->GetSponsoredImagesData(/*supports_rich_media=*/true));
  EXPECT_TRUE(observer_.on_sponsored_images_updated);
  EXPECT_THAT(observer_.sponsored_images_data->campaigns, ::testing::IsEmpty());
  EXPECT_THAT(service_->sponsored_images_data_->campaigns,
              ::testing::IsEmpty());
}

TEST_F(NTPBackgroundImagesServiceTest, SponsoredImageWithMissingImageUrlTest) {
  Init();

  observer_.sponsored_images_data = nullptr;
  service_->sponsored_images_data_.reset();
  observer_.on_sponsored_images_updated = false;
  service_->OnGetSponsoredComponentJsonData(
      kTestSponsoredImagesWithMissingImageUrl);

  EXPECT_FALSE(service_->GetSponsoredImagesData(/*supports_rich_media=*/true));
  EXPECT_TRUE(observer_.on_sponsored_images_updated);
  EXPECT_THAT(observer_.sponsored_images_data->campaigns, ::testing::IsEmpty());
  EXPECT_THAT(service_->sponsored_images_data_->campaigns,
              ::testing::IsEmpty());
}

}  // namespace ntp_background_images
