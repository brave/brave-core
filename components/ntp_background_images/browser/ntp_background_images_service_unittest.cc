/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "base/base_paths.h"
#include "base/check.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_reader.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/components/brave_component_updater/browser/mock_on_demand_updater.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service_waiter.h"
#include "brave/components/ntp_background_images/browser/sponsored_content/new_tab_takeover/ntp_sponsored_content_data.h"
#include "brave/components/ntp_background_images/browser/sponsored_content/site/ntp_sponsored_sites_data.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/sponsored_content/test/fake_sponsored_content_service.h"
#include "build/build_config.h"
#include "components/component_updater/mock_component_updater_service.h"
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
              "creativeSetId": "34ab06be-c9ed-4104-9ce0-9e639f4ad272",
              "creatives": [
                {
                  "creativeInstanceId": "3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4",
                  "companyName": "Image NTT Creative",
                  "alt": "Some image content",
                  "targetUrl": "https://brave.com",
                  "wallpaper": {
                    "type": "image",
                    "relativeUrl": "3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4/background.jpg",
                    "focalPoint": {
                      "x": 25,
                      "y": 50
                    },
                    "button": {
                      "image": {
                        "relativeUrl": "3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4/button.png"
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
              "creativeSetId": "34ab06be-c9ed-4104-9ce0-9e639f4ad272",
              "creatives": [
                {
                  "creativeInstanceId": "3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4",
                  "companyName": "Image NTT Creative",
                  "alt": "Some image content",
                  "targetUrl": "https://brave.com",
                  "wallpaper": {
                    "type": "image",
                    "relativeUrl": "3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4/background.jpg",
                    "focalPoint": {
                      "x": 25,
                      "y": 50
                    },
                    "button": {
                      "image": {
                        "relativeUrl": "3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4/button.png"
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
                  "creativeInstanceId": "aa0b561e-9eed-4aaa-8999-5627bc6b14fd",
                  "alt": "Some rich content",
                  "companyName": "Rich Media NTT Creative",
                  "targetUrl": "https://brave.com",
                  "wallpaper": {
                    "type": "richMedia",
                    "relativeUrl": "aa0b561e-9eed-4aaa-8999-5627bc6b14fd/index.html"
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
              "creativeSetId": "34ab06be-c9ed-4104-9ce0-9e639f4ad272",
              "creatives": [
                {
                  "creativeInstanceId": "3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4",
                  "companyName": "Image NTT Creative",
                  "alt": "Some image content",
                  "targetUrl": "https://brave.com",
                  "wallpaper": {
                    "type": "image",
                    "missing_relativeUrl": "3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4/background.jpg",
                    "focalPoint": {
                      "x": 25,
                      "y": 50
                    },
                    "button": {
                      "image": {
                        "relativeUrl": "3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4/button.png"
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

#if BUILDFLAG(ENABLE_BRAVE_ADS)
constexpr char kSponsoredImageContentWithNonHttpsSchemeTargetUrl[] = R"(
    {
      "schemaVersion": 2,
      "campaigns": [
        {
          "version": 1,
          "campaignId": "65933e82-6b21-440b-9956-c0f675ca7435",
          "creativeSets": [
            {
              "creativeSetId": "34ab06be-c9ed-4104-9ce0-9e639f4ad272",
              "creatives": [
                {
                  "creativeInstanceId": "3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4",
                  "companyName": "Image NTT Creative",
                  "alt": "Some image content",
                  "targetUrl": "http://basicattentiontoken.org",
                  "wallpaper": {
                    "type": "image",
                    "relativeUrl": "3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4/background.jpg",
                    "focalPoint": {
                      "x": 25,
                      "y": 50
                    },
                    "button": {
                      "image": {
                        "relativeUrl": "3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4/button.png"
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
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)

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
              "creativeSetId": "34ab06be-c9ed-4104-9ce0-9e639f4ad272",
              "creatives": [
                {
                  "creativeInstanceId": "3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4",
                  "companyName": "Image NTT Creative",
                  "alt": "Some image content",
                  "targetUrl": "https://brave.com",
                  "wallpaper": {
                    "type": "image",
                    "relativeUrl": "../background.jpg",
                    "focalPoint": {
                      "x": 25,
                      "y": 50
                    },
                    "button": {
                      "image": {
                        "relativeUrl": "3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4/button.png"
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
              "creativeSetId": "34ab06be-c9ed-4104-9ce0-9e639f4ad272",
              "creatives": [
                {
                  "creativeInstanceId": "3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4",
                  "companyName": "Image NTT Creative",
                  "alt": "Some image content",
                  "targetUrl": "https://brave.com",
                  "wallpaper": {
                    "type": "image",
                    "relativeUrl": "3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4/background.jpg",
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
                  "creativeSetId": "34ab06be-c9ed-4104-9ce0-9e639f4ad272",
                  "creatives": [
                    {
                      "creativeInstanceId": "aa0b561e-9eed-4aaa-8999-5627bc6b14fd",
                      "alt": "Some rich content",
                      "companyName": "Rich Media NTT Creative",
                      "targetUrl": "https://brave.com",
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

  void DeprecatedOnSponsoredContentDidUpdate(
      NTPSponsoredContentData* data) override {
    on_sponsored_content_updated = true;
    sponsored_content_data = data;
  }

  void OnSponsoredSitesDataDidUpdate() override {
    on_sponsored_sites_data_updated = true;
  }

  void OnSponsoredContentDidUpdate(const base::DictValue& data) override {
    sponsored_content_dict_ = data.Clone();
  }

  void Reset() {
    on_background_images_updated = false;
    background_images_data = nullptr;
    on_sponsored_content_updated = false;
    sponsored_content_data = nullptr;
    sponsored_content_dict_.reset();
    on_sponsored_sites_data_updated = false;
  }

  const std::optional<base::DictValue>& sponsored_content_dict() const {
    return sponsored_content_dict_;
  }

  raw_ptr<NTPBackgroundImagesData> background_images_data = nullptr;
  bool on_background_images_updated = false;

  raw_ptr<NTPSponsoredContentData, DanglingUntriaged> sponsored_content_data =
      nullptr;
  bool on_sponsored_content_updated = false;

  bool on_sponsored_sites_data_updated = false;

 private:
  std::optional<base::DictValue> sponsored_content_dict_;
};

class NTPBackgroundImagesServiceForTesting : public NTPBackgroundImagesService {
 public:
  using NTPBackgroundImagesService::NTPBackgroundImagesService;

  void RegisterSponsoredImagesComponent() override {
    sponsored_images_component_started = true;
    NTPBackgroundImagesService::RegisterSponsoredImagesComponent();
  }

  void RegisterBackgroundImagesComponent() override {
    background_images_component_started = true;
    NTPBackgroundImagesService::RegisterBackgroundImagesComponent();
  }

  void SetSponsoredImagesInstalledDirForTesting(const base::FilePath& dir) {
    sponsored_content_installed_dir_ = dir;
  }

  bool sponsored_images_component_started = false;
  bool background_images_component_started = false;
};

class NTPBackgroundImagesServiceTest : public testing::Test {
 public:
  NTPBackgroundImagesServiceTest() = default;

  void SetUp() override {
    PrefRegistrySimple* const pref_registry = pref_service_.registry();
    NTPBackgroundImagesService::RegisterLocalStatePrefsForMigration(
        pref_registry);
    install_dir_ = base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
                       .AppendASCII("brave")
                       .AppendASCII("test")
                       .AppendASCII("data")
                       .AppendASCII("components")
                       .AppendASCII("ntp_sponsored_images")
                       .AppendASCII("new_tab_takeover")
                       .AppendASCII("static_and_dynamic");
  }

  void TearDown() override {
    if (service_) {
      service_->RemoveObserver(&observer_);
    }
  }

  void Init() {
    component_update_service_ =
        std::make_unique<component_updater::MockComponentUpdateService>();
    ON_CALL(component_update_service(), RegisterComponent)
        .WillByDefault(testing::Return(true));
    sponsored_content_service_ = std::make_unique<
        sponsored_content::test::FakeSponsoredContentService>();
    service_ = std::make_unique<NTPBackgroundImagesServiceForTesting>(
        component_update_service_.get(), sponsored_content_service_.get(),
        &pref_service_);
    service_->Init();
    service_->AddObserver(&observer_);
  }

  component_updater::MockComponentUpdateService& component_update_service() {
    return *component_update_service_;
  }

  // Delivers `json` to the service as if it were the current `campaigns.json`
  // for `install_dir_`.
  void NotifyCampaignContent(std::string_view json) {
    std::optional<base::DictValue> dict =
        base::JSONReader::ReadDict(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
    CHECK(dict);

    // Filtering campaigns checks referenced creative files on disk, so it
    // runs on the `ThreadPool`; wait for it to reply.
    NTPBackgroundImagesServiceWaiter waiter(*service_);
    sponsored_content_service_->NotifySponsoredNewTabTakeoverContent(
        install_dir_, *dict);
    waiter.WaitForOnSponsoredContentDidUpdate();
  }

  base::FilePath SetUpSponsoredSiteWithImage() {
    // Sponsored content only loads once something has subscribed to it.
    service_->RegisterSponsoredImagesComponent();

    CHECK(installed_dir_.CreateUniqueTempDir());
    const base::FilePath image_dir =
        installed_dir_.GetPath().AppendASCII("tiles");
    CHECK(base::CreateDirectory(image_dir));
    CHECK(base::WriteFile(image_dir.AppendASCII("image.webp"), ""));

    const base::DictValue dict = base::test::ParseJsonDict(R"(
        {
          "schemaVersion": 1,
          "tiles": [
            {
              "version": 1,
              "title": "tiles",
              "adDisclosure": "Sponsored",
              "targetUrl": "https://foo.com",
              "image": {
                "relativeUrl": "tiles/image.webp"
              }
            }
          ]
        })");
    // Parsing sponsored sites checks referenced image files on disk, so it
    // runs on the `ThreadPool`; wait for it to reply.
    NTPBackgroundImagesServiceWaiter waiter(*service_);
    sponsored_content_service_->NotifySponsoredSiteContent(
        installed_dir_.GetPath(), dict);
    waiter.WaitForOnSponsoredSitesDataDidUpdate();

    return image_dir;
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  base::FilePath install_dir_;
  TestingPrefServiceSimple pref_service_;
  // Registers itself with `BraveOnDemandUpdater`, so draining the task
  // queue does not crash on a real component registration.
  brave_component_updater::MockOnDemandUpdater on_demand_updater_;
  std::unique_ptr<component_updater::MockComponentUpdateService>
      component_update_service_;
  std::unique_ptr<sponsored_content::test::FakeSponsoredContentService>
      sponsored_content_service_;
  std::unique_ptr<NTPBackgroundImagesServiceForTesting> service_;
  ObserverMock observer_;
  base::ScopedTempDir installed_dir_;
};

TEST_F(NTPBackgroundImagesServiceTest, BasicTest) {
  Init();
  // The sponsored content component is registered after ads is initialized.
  EXPECT_FALSE(service_->sponsored_images_component_started);
  // If ENABLE_NTP_BACKGROUND_IMAGES then the sponsored backgrounds component
  // shall be registered.
  EXPECT_TRUE(service_->background_images_component_started);
}

TEST_F(NTPBackgroundImagesServiceTest, InternalDataTest) {
  Init();

  // Check with json file w/o schema version with empty object.
  service_->sponsored_content_data_.reset();

  service_->RegisterSponsoredImagesComponent();
  NotifyCampaignContent("{}");
  EXPECT_FALSE(service_->GetNewTabTakeover(
      /*supports_dynamic_new_tab_takeover=*/true));
  service_->background_images_data_.reset();
  service_->OnGetComponentJsonData("{}");
  EXPECT_FALSE(service_->GetBackgroundImagesData());

  // Check with json file with empty object.
  observer_.sponsored_content_data = nullptr;
  service_->sponsored_content_data_.reset();
  observer_.on_sponsored_content_updated = false;
  NotifyCampaignContent(kTestEmptyComponent);
  EXPECT_FALSE(
      service_->GetNewTabTakeover(/*supports_dynamic_new_tab_takeover=*/true));
  EXPECT_TRUE(observer_.on_sponsored_content_updated);
  EXPECT_THAT(observer_.sponsored_content_data->campaigns,
              ::testing::IsEmpty());
  observer_.background_images_data = nullptr;
  service_->background_images_data_.reset();
  observer_.on_background_images_updated = false;
  service_->OnGetComponentJsonData(kTestEmptyComponent);
  NTPBackgroundImagesData* background_images_data =
      service_->GetBackgroundImagesData();
  EXPECT_FALSE(background_images_data);
  EXPECT_TRUE(observer_.on_background_images_updated);
  EXPECT_FALSE(observer_.background_images_data->IsValid());

  observer_.sponsored_content_data = nullptr;
  service_->sponsored_content_data_.reset();
  observer_.on_sponsored_content_updated = false;
  NotifyCampaignContent(kTestSponsoredImages);
  NTPSponsoredContentData* const sponsored_content_data =
      service_->GetNewTabTakeover(/*supports_dynamic_new_tab_takeover=*/true);
  EXPECT_TRUE(sponsored_content_data);
  EXPECT_TRUE(sponsored_content_data->IsValid());
  // Above json data has 3 wallpapers.
  EXPECT_THAT(sponsored_content_data->campaigns, ::testing::SizeIs(1));
  const Campaign campaign = sponsored_content_data->campaigns[0];
  EXPECT_THAT(campaign.id, ::testing::Not(::testing::IsEmpty()));
  EXPECT_THAT(campaign.creatives, ::testing::SizeIs(1));
  EXPECT_EQ(25, campaign.creatives[0].focal_point.x());
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("background.jpg"),
            campaign.creatives[0].file_path.BaseName());
  EXPECT_EQ(campaign.creatives[0].creative_instance_id,
            "3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4");
  EXPECT_TRUE(observer_.on_sponsored_content_updated);
  EXPECT_THAT(
      observer_.sponsored_content_data->campaigns[0].creatives[0].logo.alt_text,
      ::testing::Not(::testing::IsEmpty()));
  EXPECT_TRUE(sponsored_content_data->MaybeGetBackgroundAt(0, 0)->FindBool(
      kIsSponsoredKey));
  EXPECT_FALSE(sponsored_content_data->MaybeGetBackgroundAt(0, 0)
                   ->FindBool(kIsBackgroundKey)
                   .value());

  EXPECT_EQ(install_dir_.AppendASCII("3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4")
                .AppendASCII("button.png")
                .AsUTF8Unsafe(),
            *sponsored_content_data->MaybeGetBackgroundAt(0, 0)
                 ->FindStringByDottedPath(kLogoImagePath));

  // Test sponsored backgrounds data loading.
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
              "creativeSetId": "34ab06be-c9ed-4104-9ce0-9e639f4ad272",
              "creatives": [
                {
                  "creativeInstanceId": "3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4",
                  "companyName": "Image NTT Creative",
                  "alt": "Some image content",
                  "targetUrl": "https://brave.com",
                  "wallpaper": {
                    "type": "image",
                    "relativeUrl": "3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4/background.jpg",
                    "focalPoint": {
                      "x": 25,
                      "y": 50
                    },
                    "button": {
                      "image": {
                        "relativeUrl": "3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4/button.png"
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
  observer_.sponsored_content_data = nullptr;
  service_->sponsored_content_data_.reset();
  observer_.on_sponsored_content_updated = false;
  NotifyCampaignContent(test_json_string_higher_schema);
  EXPECT_FALSE(
      service_->GetNewTabTakeover(/*supports_dynamic_new_tab_takeover=*/true));

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

  observer_.sponsored_content_data = nullptr;
  service_->sponsored_content_data_.reset();
  observer_.on_sponsored_content_updated = false;
  service_->RegisterSponsoredImagesComponent();
  NotifyCampaignContent(kTestSponsoredImagesWithMultipleCampaigns);
  const NTPSponsoredContentData* const sponsored_content_data =
      service_->GetNewTabTakeover(/*supports_dynamic_new_tab_takeover=*/true);
  EXPECT_TRUE(sponsored_content_data);
  EXPECT_TRUE(sponsored_content_data->IsValid());
  EXPECT_THAT(sponsored_content_data->campaigns, ::testing::SizeIs(2));
  const Campaign campaign_0 = sponsored_content_data->campaigns[0];
  EXPECT_THAT(campaign_0.id, ::testing::Not(::testing::IsEmpty()));
  EXPECT_THAT(campaign_0.creatives, ::testing::SizeIs(1));
  EXPECT_THAT(campaign_0.creatives[0].creative_instance_id,
              ::testing::Not(::testing::IsEmpty()));
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("background.jpg"),
            campaign_0.creatives[0].file_path.BaseName());
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("button.png"),
            campaign_0.creatives[0].logo.image_file.BaseName());

  const Campaign campaign_1 = sponsored_content_data->campaigns[1];
  EXPECT_THAT(campaign_1.id, ::testing::Not(::testing::IsEmpty()));
  EXPECT_THAT(campaign_1.creatives, ::testing::SizeIs(1));
  EXPECT_THAT(campaign_1.creatives[0].creative_instance_id,
              ::testing::Not(::testing::IsEmpty()));
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("index.html"),
            campaign_1.creatives[0].file_path.BaseName());
}

#if BUILDFLAG(ENABLE_BRAVE_ADS)
TEST_F(NTPBackgroundImagesServiceTest,
       DoNotGetSponsoredImageContentForNonHttpsSchemeTargetUrl) {
  Init();

  observer_.sponsored_content_data = nullptr;
  service_->sponsored_content_data_.reset();
  observer_.on_sponsored_content_updated = false;
  service_->RegisterSponsoredImagesComponent();
  NotifyCampaignContent(kSponsoredImageContentWithNonHttpsSchemeTargetUrl);

  EXPECT_FALSE(
      service_->GetNewTabTakeover(/*supports_dynamic_new_tab_takeover=*/true));
  EXPECT_TRUE(observer_.on_sponsored_content_updated);
  EXPECT_THAT(observer_.sponsored_content_data->campaigns,
              ::testing::IsEmpty());
  EXPECT_THAT(service_->sponsored_content_data_->campaigns,
              ::testing::IsEmpty());
}
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)

TEST_F(NTPBackgroundImagesServiceTest,
       DoNotGetSponsoredImageContentIfWallpaperUrlReferencesParent) {
  Init();

  observer_.sponsored_content_data = nullptr;
  service_->sponsored_content_data_.reset();
  observer_.on_sponsored_content_updated = false;
  service_->RegisterSponsoredImagesComponent();
  NotifyCampaignContent(
      kSponsoredImageContentWithWallpaperRelativeUrlReferencingParent);

  EXPECT_FALSE(
      service_->GetNewTabTakeover(/*supports_dynamic_new_tab_takeover=*/true));
  EXPECT_TRUE(observer_.on_sponsored_content_updated);
  EXPECT_THAT(observer_.sponsored_content_data->campaigns,
              ::testing::IsEmpty());
  EXPECT_THAT(service_->sponsored_content_data_->campaigns,
              ::testing::IsEmpty());
}

TEST_F(
    NTPBackgroundImagesServiceTest,
    DoNotGetSponsoredImageContentIfWallpaperButtonImageRelativeUrlReferencesParent) {
  Init();

  observer_.sponsored_content_data = nullptr;
  service_->sponsored_content_data_.reset();
  observer_.on_sponsored_content_updated = false;
  service_->RegisterSponsoredImagesComponent();
  NotifyCampaignContent(
      kSponsoredImageContentWithWallpaperButtonImageRelativeUrlReferencingParent);

  EXPECT_FALSE(
      service_->GetNewTabTakeover(/*supports_dynamic_new_tab_takeover=*/true));
  EXPECT_TRUE(observer_.on_sponsored_content_updated);
  EXPECT_THAT(observer_.sponsored_content_data->campaigns,
              ::testing::IsEmpty());
  EXPECT_THAT(service_->sponsored_content_data_->campaigns,
              ::testing::IsEmpty());
}

TEST_F(
    NTPBackgroundImagesServiceTest,
    DoNotGetSponsoredRichMediaContentIfWallpaperRelativeUrlReferencesParent) {
  Init();

  observer_.sponsored_content_data = nullptr;
  service_->sponsored_content_data_.reset();
  observer_.on_sponsored_content_updated = false;
  service_->RegisterSponsoredImagesComponent();
  NotifyCampaignContent(
      kSponsoredRichMediaContentWithWallpaperRelativeUrlReferencingParent);

  EXPECT_FALSE(
      service_->GetNewTabTakeover(/*supports_dynamic_new_tab_takeover=*/true));
  EXPECT_TRUE(observer_.on_sponsored_content_updated);
  EXPECT_THAT(observer_.sponsored_content_data->campaigns,
              ::testing::IsEmpty());
  EXPECT_THAT(service_->sponsored_content_data_->campaigns,
              ::testing::IsEmpty());
}

TEST_F(NTPBackgroundImagesServiceTest, SponsoredImageWithMissingImageUrlTest) {
  Init();

  observer_.sponsored_content_data = nullptr;
  service_->sponsored_content_data_.reset();
  observer_.on_sponsored_content_updated = false;
  service_->RegisterSponsoredImagesComponent();
  NotifyCampaignContent(kTestSponsoredImagesWithMissingImageUrl);

  EXPECT_FALSE(
      service_->GetNewTabTakeover(/*supports_dynamic_new_tab_takeover=*/true));
  EXPECT_TRUE(observer_.on_sponsored_content_updated);
  EXPECT_THAT(observer_.sponsored_content_data->campaigns,
              ::testing::IsEmpty());
  EXPECT_THAT(service_->sponsored_content_data_->campaigns,
              ::testing::IsEmpty());
}

TEST_F(NTPBackgroundImagesServiceTest,
       ReplaysNotificationForProfilesOpenedAfterInitialLoad) {
  Init();

  service_->RegisterSponsoredImagesComponent();
  NotifyCampaignContent(kTestSponsoredImages);
  ASSERT_TRUE(observer_.on_sponsored_content_updated);

  // Simulate a second profile opening after the component was already loaded.
  ObserverMock second_observer;
  service_->AddObserver(&second_observer);
  ASSERT_FALSE(second_observer.on_sponsored_content_updated);

  NTPBackgroundImagesServiceWaiter waiter(*service_);
  service_->RegisterSponsoredImagesComponent();
  waiter.WaitForOnSponsoredContentDidUpdate();

  EXPECT_TRUE(second_observer.on_sponsored_content_updated);
  EXPECT_TRUE(second_observer.sponsored_content_data);

  service_->RemoveObserver(&second_observer);
}

TEST_F(NTPBackgroundImagesServiceTest,
       DoesNotReplayBeforeAnyContentEverLoaded) {
  Init();

  service_->RegisterSponsoredImagesComponent();
  observer_.Reset();

  service_->RegisterSponsoredImagesComponent();
  EXPECT_FALSE(observer_.on_sponsored_content_updated);
}

TEST_F(NTPBackgroundImagesServiceTest,
       GetsSponsoredSitesDataAndNotifiesObserverWhenValid) {
  Init();

  ASSERT_FALSE(service_->GetSponsoredSitesData());
  ASSERT_FALSE(observer_.on_sponsored_sites_data_updated);

  const base::FilePath image_dir = SetUpSponsoredSiteWithImage();

  const NTPSponsoredSitesData* const fetched_sites_data =
      service_->GetSponsoredSitesData();
  ASSERT_TRUE(fetched_sites_data);
  EXPECT_THAT(fetched_sites_data->sites,
              ::testing::ElementsAre(
                  ::testing::Field(&NTPSponsoredSite::title, "tiles")));
  EXPECT_TRUE(observer_.on_sponsored_sites_data_updated);
}

TEST_F(NTPBackgroundImagesServiceTest,
       ClearsSponsoredSitesDataAndStillNotifiesObserverWhenInvalid) {
  Init();

  SetUpSponsoredSiteWithImage();
  ASSERT_TRUE(service_->GetSponsoredSitesData());

  observer_.on_sponsored_sites_data_updated = false;
  NTPBackgroundImagesServiceWaiter waiter(*service_);
  sponsored_content_service_->NotifySponsoredSiteContent(
      installed_dir_.GetPath(), base::test::ParseJsonDict("{}"));
  waiter.WaitForOnSponsoredSitesDataDidUpdate();

  EXPECT_FALSE(service_->GetSponsoredSitesData());
  EXPECT_TRUE(observer_.on_sponsored_sites_data_updated);
}

TEST_F(NTPBackgroundImagesServiceTest,
       MaybeGetSponsoredSiteImageFilePathReturnsPathForKnownSiteImage) {
  Init();

  const base::FilePath image_dir = SetUpSponsoredSiteWithImage();

  std::optional<base::FilePath> result =
      service_->MaybeGetSponsoredSiteImageFilePath(
          base::FilePath::FromUTF8Unsafe("tiles/image.webp"));

  ASSERT_TRUE(result);
  EXPECT_EQ(image_dir.AppendASCII("image.webp"), *result);
}

TEST_F(NTPBackgroundImagesServiceTest,
       MaybeGetSponsoredSiteImageFilePathRejectsUnknownImage) {
  Init();

  SetUpSponsoredSiteWithImage();

  EXPECT_FALSE(service_->MaybeGetSponsoredSiteImageFilePath(
      base::FilePath::FromUTF8Unsafe("tiles/unknown.webp")));
}

TEST_F(NTPBackgroundImagesServiceTest,
       MaybeGetSponsoredSiteImageFilePathRejectsPathTraversal) {
  Init();

  SetUpSponsoredSiteWithImage();

  EXPECT_FALSE(service_->MaybeGetSponsoredSiteImageFilePath(
      base::FilePath::FromUTF8Unsafe("../tiles/image.webp")));
}

TEST_F(
    NTPBackgroundImagesServiceTest,
    MaybeGetSponsoredSiteImageFilePathReturnsNulloptWhenNoSponsoredSitesData) {
  Init();

  base::ScopedTempDir installed_dir;
  ASSERT_TRUE(installed_dir.CreateUniqueTempDir());
  service_->SetSponsoredImagesInstalledDirForTesting(installed_dir.GetPath());

  EXPECT_FALSE(service_->MaybeGetSponsoredSiteImageFilePath(
      base::FilePath::FromUTF8Unsafe("tiles/image.webp")));
}

TEST_F(NTPBackgroundImagesServiceTest,
       OnlyUnregistersSponsoredImagesComponentWhenNoProfileRemainsOptedIn) {
  Init();

  // Two profiles opt in. Only the first one should trigger registration.
  service_->AddSponsoredImagesOptedInProfile();
  service_->AddSponsoredImagesOptedInProfile();
  EXPECT_EQ(1U, sponsored_content_service_->register_call_count());

  // One profile opts out, but the other profile is still opted in, so the
  // component must remain registered.
  service_->RemoveSponsoredImagesOptedInProfile();
  EXPECT_EQ(0U, sponsored_content_service_->unregister_call_count());

  // The last opted-in profile opts out, so the component is unregistered.
  service_->RemoveSponsoredImagesOptedInProfile();
  EXPECT_EQ(1U, sponsored_content_service_->unregister_call_count());
}

}  // namespace ntp_background_images
