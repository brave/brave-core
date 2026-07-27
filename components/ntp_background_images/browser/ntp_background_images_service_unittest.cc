/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/base_paths.h"
#include "base/check.h"
#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_reader.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_component_updater/browser/mock_on_demand_updater.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_sites_data.h"
#include "brave/components/ntp_background_images/browser/sponsored_images_component_data.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "build/build_config.h"
#include "components/component_updater/mock_component_updater_service.h"
#include "components/prefs/testing_pref_service.h"
#include "components/update_client/update_client.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

constexpr char kTestSponsoredSitesUrlPrefix[] =
    "chrome://sponsored-site-image/";

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

constexpr char kTestRichMedia[] = R"(
      {
        "schemaVersion": 2,
        "campaigns": [
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

std::string GetComponentId(std::string_view country_code) {
  std::optional<SponsoredImagesComponentInfo> component =
      GetSponsoredImagesComponent(country_code);
  CHECK(component);
  return std::string(component->id);
}

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

  void OnSponsoredSitesDataDidUpdate() override {
    on_sponsored_sites_data_updated = true;
  }

  void OnSponsoredContentDidUpdate(const base::DictValue& data) override {
    sponsored_content_data_ = data.Clone();
  }

  void Reset() {
    on_background_images_updated = false;
    background_images_data = nullptr;
    on_sponsored_images_updated = false;
    sponsored_images_data = nullptr;
    sponsored_content_data_.reset();
  }

  const std::optional<base::DictValue>& sponsored_content_data() {
    return sponsored_content_data_;
  }

  raw_ptr<NTPBackgroundImagesData> background_images_data = nullptr;
  bool on_background_images_updated = false;

  raw_ptr<NTPSponsoredImagesData, DanglingUntriaged> sponsored_images_data =
      nullptr;
  bool on_sponsored_images_updated = false;

  bool on_sponsored_sites_data_updated = false;

 private:
  std::optional<base::DictValue> sponsored_content_data_;
};

class NTPBackgroundImagesServiceForTesting : public NTPBackgroundImagesService {
 public:
  using NTPBackgroundImagesService::NTPBackgroundImagesService;

  void RegisterSponsoredImagesComponent() override {
    sponsored_images_component_ready_ = false;
    on_handled_sponsored_component_data_called_ = false;
    sponsored_images_component_started = true;
    NTPBackgroundImagesService::RegisterSponsoredImagesComponent();
  }

  void RegisterBackgroundImagesComponent() override {
    background_images_component_started = true;
    NTPBackgroundImagesService::RegisterBackgroundImagesComponent();
  }

  std::string GetCountryCode() const override { return country_code_; }

  void HandleSponsoredComponentData(const base::FilePath& installed_dir,
                                    const std::string& json) {
    SetSponsoredImagesInstalledDirForTesting(installed_dir);
    NTPBackgroundImagesService::OnHandledSponsoredComponentData(
        base::JSONReader::ReadDict(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS));
  }

  // Pre-sets the already-loaded state so tests can call
  // RegisterSponsoredImagesComponent and exercise the replay path.
  void SetSponsoredImagesLoadedForTesting(const base::FilePath& installed_dir,
                                          const std::string& json) {
    SetSponsoredComponentJsonForTesting(GetComponentId(country_code_), json);
    HandleSponsoredComponentData(installed_dir, json);
  }

  // Pre-sets the sponsored component json which will be loaded for the given
  // component id.
  void SetSponsoredComponentJsonForTesting(const std::string& component_id,
                                           const std::string& json) {
    component_json_for_testing_[component_id] = json;
  }

  void SetCountryCode(std::string country_code) {
    country_code_ = std::move(country_code);
  }

  void OnSponsoredComponentReady(const base::FilePath& installed_dir) override {
    sponsored_images_component_ready_ = true;
    NTPBackgroundImagesService::OnSponsoredComponentReady(installed_dir);
  }

  std::optional<std::string> sponsored_images_component_id() const {
    return sponsored_images_component_id_;
  }

  bool sponsored_images_component_ready() const {
    return sponsored_images_component_ready_;
  }

  bool on_handled_sponsored_component_data_called() const {
    return on_handled_sponsored_component_data_called_;
  }

  void OnGetSponsoredSitesData(
      std::optional<NTPSponsoredSitesData> sites_data) {
    NTPBackgroundImagesService::OnHandledSponsoredSitesData(
        std::move(sites_data));
  }

  void SetSponsoredImagesInstalledDirForTesting(const base::FilePath& dir) {
    sponsored_images_installed_dir_ = dir;
  }

  bool sponsored_images_component_started = false;
  bool background_images_component_started = false;

 private:
  void OnHandledSponsoredComponentData(
      std::optional<base::DictValue> dict) override {
    on_handled_sponsored_component_data_called_ = true;

    CHECK(sponsored_images_component_id());
    NTPBackgroundImagesService::OnHandledSponsoredComponentData(
        base::JSONReader::ReadDict(
            component_json_for_testing_[*sponsored_images_component_id()],
            base::JSON_PARSE_CHROMIUM_EXTENSIONS));
  }

  base::flat_map<std::string, std::string> component_json_for_testing_;
  std::string country_code_ = "US";
  bool sponsored_images_component_ready_ = false;
  bool on_handled_sponsored_component_data_called_ = false;
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
                       .AppendASCII("image_and_rich_media");
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
    service_ = std::make_unique<NTPBackgroundImagesServiceForTesting>(
        /*variations_service=*/nullptr, component_update_service_.get(),
        &pref_service_);
    service_->Init();
    service_->AddObserver(&observer_);

    ON_CALL(on_demand_updater_, OnDemandUpdate(testing::A<const std::string&>(),
                                               testing::_, testing::_))
        .WillByDefault([this](auto component_id, auto priority, auto callback) {
          std::move(callback).Run(update_client::Error::NONE);
          service_->OnSponsoredComponentReady(install_dir_);
        });
  }

  component_updater::MockComponentUpdateService& component_update_service() {
    return *component_update_service_;
  }

  base::FilePath SetUpSponsoredSiteWithImage() {
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
    NTPSponsoredSitesData sites_data(dict, installed_dir_.GetPath(),
                                     kTestSponsoredSitesUrlPrefix);
    CHECK(sites_data.IsValid());
    service_->OnGetSponsoredSitesData(std::move(sites_data));

    return image_dir;
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  base::FilePath install_dir_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<component_updater::MockComponentUpdateService>
      component_update_service_;
  std::unique_ptr<NTPBackgroundImagesServiceForTesting> service_;
  ObserverMock observer_;
  base::ScopedTempDir installed_dir_;
  brave_component_updater::MockOnDemandUpdater on_demand_updater_;
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

  service_->RegisterSponsoredImagesComponent();
  service_->HandleSponsoredComponentData(install_dir_, "{}");
  EXPECT_FALSE(service_->GetSponsoredImagesData(
      /*supports_rich_media=*/true));
  service_->background_images_data_.reset();
  service_->OnGetComponentJsonData("{}");
  EXPECT_FALSE(service_->GetBackgroundImagesData());

  // Check with json file with empty object.
  observer_.sponsored_images_data = nullptr;
  service_->sponsored_images_data_.reset();
  observer_.on_sponsored_images_updated = false;
  service_->HandleSponsoredComponentData(install_dir_, kTestEmptyComponent);
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
  service_->HandleSponsoredComponentData(install_dir_, kTestSponsoredImages);
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
            "3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4");
  EXPECT_TRUE(observer_.on_sponsored_images_updated);
  EXPECT_THAT(
      observer_.sponsored_images_data->campaigns[0].creatives[0].logo.alt_text,
      ::testing::Not(::testing::IsEmpty()));
  EXPECT_TRUE(
      images_data->MaybeGetBackgroundAt(0, 0)->FindBool(kIsSponsoredKey));
  EXPECT_FALSE(images_data->MaybeGetBackgroundAt(0, 0)
                   ->FindBool(kIsBackgroundKey)
                   .value());

  EXPECT_EQ(install_dir_.AppendASCII("3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4")
                .AppendASCII("button.png")
                .AsUTF8Unsafe(),
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
  observer_.sponsored_images_data = nullptr;
  service_->sponsored_images_data_.reset();
  observer_.on_sponsored_images_updated = false;
  service_->HandleSponsoredComponentData(install_dir_,
                                         test_json_string_higher_schema);
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
  service_->RegisterSponsoredImagesComponent();
  service_->SetSponsoredImagesLoadedForTesting(
      install_dir_, kTestSponsoredImagesWithMultipleCampaigns);
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
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("background.jpg"),
            campaign_0.creatives[0].file_path.BaseName());
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("button.png"),
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
  service_->RegisterSponsoredImagesComponent();
  service_->SetSponsoredImagesLoadedForTesting(
      install_dir_, kSponsoredImageContentWithNonHttpsSchemeTargetUrl);

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
  service_->RegisterSponsoredImagesComponent();
  service_->HandleSponsoredComponentData(
      install_dir_,
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
  service_->RegisterSponsoredImagesComponent();
  service_->HandleSponsoredComponentData(
      install_dir_,
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
  service_->RegisterSponsoredImagesComponent();
  service_->HandleSponsoredComponentData(
      install_dir_,
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
  service_->RegisterSponsoredImagesComponent();
  service_->HandleSponsoredComponentData(
      install_dir_, kTestSponsoredImagesWithMissingImageUrl);

  EXPECT_FALSE(service_->GetSponsoredImagesData(/*supports_rich_media=*/true));
  EXPECT_TRUE(observer_.on_sponsored_images_updated);
  EXPECT_THAT(observer_.sponsored_images_data->campaigns, ::testing::IsEmpty());
  EXPECT_THAT(service_->sponsored_images_data_->campaigns,
              ::testing::IsEmpty());
}

TEST_F(NTPBackgroundImagesServiceTest,
       ReplaysNotificationForProfilesOpenedAfterInitialLoad) {
  Init();

  service_->RegisterSponsoredImagesComponent();
  service_->SetSponsoredImagesLoadedForTesting(install_dir_,
                                               kTestSponsoredImages);
  ASSERT_TRUE(observer_.on_sponsored_images_updated);

  // Simulate a second profile opening after the component was already loaded.
  ObserverMock second_observer;
  service_->AddObserver(&second_observer);
  ASSERT_FALSE(second_observer.on_sponsored_images_updated);

  service_->RegisterSponsoredImagesComponent();
  EXPECT_TRUE(service_->sponsored_images_component_ready());

  ASSERT_TRUE(base::test::RunUntil([this]() {
    return service_->on_handled_sponsored_component_data_called();
  }));

  EXPECT_TRUE(second_observer.on_sponsored_images_updated);
  EXPECT_TRUE(second_observer.sponsored_images_data);

  service_->RemoveObserver(&second_observer);
}

TEST_F(NTPBackgroundImagesServiceTest,
       GetsSponsoredSitesDataAndNotifiesObserverWhenValid) {
  Init();

  base::ScopedTempDir installed_dir;
  ASSERT_TRUE(installed_dir.CreateUniqueTempDir());
  const base::FilePath image_dir = installed_dir.GetPath().AppendASCII("tiles");
  ASSERT_TRUE(base::CreateDirectory(image_dir));
  ASSERT_TRUE(base::WriteFile(image_dir.AppendASCII("image.webp"), ""));

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
  NTPSponsoredSitesData sites_data(dict, installed_dir.GetPath(),
                                   kTestSponsoredSitesUrlPrefix);
  ASSERT_TRUE(sites_data.IsValid());

  ASSERT_FALSE(service_->GetSponsoredSitesData());
  ASSERT_FALSE(observer_.on_sponsored_sites_data_updated);

  service_->OnGetSponsoredSitesData(std::move(sites_data));

  const NTPSponsoredSitesData* const fetched_sites_data =
      service_->GetSponsoredSitesData();
  ASSERT_TRUE(fetched_sites_data);
  EXPECT_THAT(fetched_sites_data->sites,
              ::testing::ElementsAre(
                  ::testing::Field(&NTPSponsoredSite::title, "tiles")));
  EXPECT_TRUE(observer_.on_sponsored_sites_data_updated);
}

TEST_F(NTPBackgroundImagesServiceTest,
       ClearsSponsoredSitesDataAndStillNotifiesObserverWhenNullopt) {
  Init();

  base::ScopedTempDir installed_dir;
  ASSERT_TRUE(installed_dir.CreateUniqueTempDir());
  const base::FilePath image_dir = installed_dir.GetPath().AppendASCII("tiles");
  ASSERT_TRUE(base::CreateDirectory(image_dir));
  ASSERT_TRUE(base::WriteFile(image_dir.AppendASCII("image.webp"), ""));

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
  NTPSponsoredSitesData sites_data(dict, installed_dir.GetPath(),
                                   kTestSponsoredSitesUrlPrefix);
  ASSERT_TRUE(sites_data.IsValid());
  service_->OnGetSponsoredSitesData(std::move(sites_data));
  ASSERT_TRUE(service_->GetSponsoredSitesData());

  observer_.on_sponsored_sites_data_updated = false;
  service_->OnGetSponsoredSitesData(std::nullopt);

  EXPECT_FALSE(service_->GetSponsoredSitesData());
  EXPECT_TRUE(observer_.on_sponsored_sites_data_updated);
}

TEST_F(NTPBackgroundImagesServiceTest,
       MaybeGetSponsoredSiteImageFilePathReturnsPathForKnownSiteImage) {
  Init();

  const base::FilePath image_dir = SetUpSponsoredSiteWithImage();
  service_->SetSponsoredImagesInstalledDirForTesting(installed_dir_.GetPath());

  const std::optional<base::FilePath> result =
      service_->MaybeGetSponsoredSiteImageFilePath(
          base::FilePath::FromUTF8Unsafe("tiles/image.webp"));

  ASSERT_TRUE(result);
  EXPECT_EQ(image_dir.AppendASCII("image.webp"), *result);
}

TEST_F(NTPBackgroundImagesServiceTest,
       MaybeGetSponsoredSiteImageFilePathRejectsUnknownImage) {
  Init();

  SetUpSponsoredSiteWithImage();
  service_->SetSponsoredImagesInstalledDirForTesting(installed_dir_.GetPath());

  EXPECT_FALSE(service_->MaybeGetSponsoredSiteImageFilePath(
      base::FilePath::FromUTF8Unsafe("tiles/unknown.webp")));
}

TEST_F(NTPBackgroundImagesServiceTest,
       MaybeGetSponsoredSiteImageFilePathRejectsPathTraversal) {
  Init();

  SetUpSponsoredSiteWithImage();
  service_->SetSponsoredImagesInstalledDirForTesting(installed_dir_.GetPath());

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
       SponsoredSiteDataResetWhenCountryChangesAndNewComponentNotYetLoaded) {
  Init();

  service_->SetCountryCode("US");
  service_->RegisterSponsoredImagesComponent();
  SetUpSponsoredSiteWithImage();
  service_->SetSponsoredImagesInstalledDirForTesting(installed_dir_.GetPath());
  EXPECT_TRUE(service_->GetSponsoredSitesData());
  observer_.Reset();

  service_->SetCountryCode("GB");
  service_->RegisterSponsoredImagesComponent();

  EXPECT_TRUE(observer_.on_sponsored_sites_data_updated);
  EXPECT_FALSE(service_->GetSponsoredSitesData());
}

TEST_F(NTPBackgroundImagesServiceTest,
       SponsoredComponentInvalidJsonClearsData) {
  Init();

  service_->RegisterSponsoredImagesComponent();
  service_->SetSponsoredImagesLoadedForTesting(install_dir_,
                                               kTestSponsoredImages);
  ASSERT_TRUE(service_->GetSponsoredImagesData(/*supports_rich_media=*/true));
  observer_.Reset();

  service_->HandleSponsoredComponentData(install_dir_, "MALFORMED_JSON");

  EXPECT_FALSE(service_->GetSponsoredImagesData(/*supports_rich_media=*/true));
  EXPECT_TRUE(observer_.on_sponsored_images_updated);
  EXPECT_FALSE(observer_.sponsored_images_data);
  EXPECT_THAT(observer_.sponsored_content_data(),
              testing::Optional(::testing::IsEmpty()));
}

TEST_F(NTPBackgroundImagesServiceTest,
       ReregisterSponsoredImagesComponentWhenCountryChanges) {
  Init();

  service_->SetSponsoredComponentJsonForTesting(GetComponentId("US"),
                                                kTestSponsoredImages);
  service_->SetSponsoredComponentJsonForTesting(GetComponentId("GB"),
                                                kTestRichMedia);

  EXPECT_CALL(component_update_service(),
              UnregisterComponent(GetComponentId("US")));

  service_->SetCountryCode("US");
  service_->RegisterSponsoredImagesComponent();
  ASSERT_TRUE(base::test::RunUntil([this]() {
    return service_->on_handled_sponsored_component_data_called();
  }));
  observer_.Reset();

  service_->SetCountryCode("GB");
  service_->RegisterSponsoredImagesComponent();
  ASSERT_TRUE(base::test::RunUntil([this]() {
    return service_->on_handled_sponsored_component_data_called();
  }));

  EXPECT_TRUE(service_->GetSponsoredImagesData(/*supports_rich_media=*/true));
  EXPECT_TRUE(observer_.on_sponsored_images_updated);
  EXPECT_TRUE(observer_.sponsored_images_data);
  EXPECT_EQ(observer_.sponsored_content_data(),
            base::JSONReader::ReadDict(kTestRichMedia,
                                       base::JSON_PARSE_CHROMIUM_EXTENSIONS));
}

TEST_F(NTPBackgroundImagesServiceTest,
       SponsoredImagesDataResetWhenCountryChangesAndNewComponentNotYetLoaded) {
  Init();

  service_->SetCountryCode("US");
  service_->RegisterSponsoredImagesComponent();
  service_->SetSponsoredImagesLoadedForTesting(install_dir_,
                                               kTestSponsoredImages);
  observer_.Reset();

  service_->SetCountryCode("GB");
  service_->RegisterSponsoredImagesComponent();

  EXPECT_FALSE(service_->GetSponsoredImagesData(/*supports_rich_media=*/true));
  EXPECT_TRUE(observer_.on_sponsored_images_updated);
  EXPECT_FALSE(observer_.sponsored_images_data);
  EXPECT_THAT(observer_.sponsored_content_data(),
              testing::Optional(::testing::IsEmpty()));
}

TEST_F(NTPBackgroundImagesServiceTest,
       ReplaySponsoredImagesDataAfterComponentLoaded) {
  Init();

  service_->SetCountryCode("US");
  service_->RegisterSponsoredImagesComponent();
  service_->SetSponsoredImagesLoadedForTesting(install_dir_,
                                               kTestSponsoredImages);

  observer_.Reset();
  service_->RegisterSponsoredImagesComponent();
  EXPECT_TRUE(service_->sponsored_images_component_ready());

  ASSERT_TRUE(base::test::RunUntil([this]() {
    return service_->on_handled_sponsored_component_data_called();
  }));
  EXPECT_TRUE(observer_.on_sponsored_images_updated);
}

TEST_F(NTPBackgroundImagesServiceTest,
       DoNotReplaySponsoredImagesDataBeforeComponentLoaded) {
  Init();

  service_->SetCountryCode("US");
  service_->RegisterSponsoredImagesComponent();
  observer_.Reset();

  service_->RegisterSponsoredImagesComponent();
  EXPECT_FALSE(service_->sponsored_images_component_ready());
}

TEST_F(
    NTPBackgroundImagesServiceTest,
    DoNotReplaySponsoredImagesDataBeforeNewComponentLoadedAfterCountryChange) {
  Init();

  service_->SetCountryCode("US");
  service_->RegisterSponsoredImagesComponent();
  service_->SetSponsoredImagesLoadedForTesting(install_dir_,
                                               kTestSponsoredImages);

  service_->SetCountryCode("GB");
  service_->RegisterSponsoredImagesComponent();
  observer_.Reset();

  service_->RegisterSponsoredImagesComponent();
  EXPECT_FALSE(service_->sponsored_images_component_ready());
}

TEST_F(NTPBackgroundImagesServiceTest,
       StaleCallbackFromPreviousComponentIsDiscardedWhenCountryChanges) {
  Init();

  service_->SetSponsoredComponentJsonForTesting(GetComponentId("US"),
                                                kTestSponsoredImages);
  service_->SetSponsoredComponentJsonForTesting(GetComponentId("GB"),
                                                kTestRichMedia);

  service_->SetCountryCode("US");
  service_->RegisterSponsoredImagesComponent();

  service_->SetCountryCode("GB");
  service_->RegisterSponsoredImagesComponent();

  ASSERT_TRUE(base::test::RunUntil([this]() {
    return service_->on_handled_sponsored_component_data_called();
  }));

  EXPECT_TRUE(service_->GetSponsoredImagesData(/*supports_rich_media=*/true));
  EXPECT_TRUE(observer_.on_sponsored_images_updated);
  EXPECT_TRUE(observer_.sponsored_images_data);
  EXPECT_EQ(observer_.sponsored_content_data(),
            base::JSONReader::ReadDict(kTestRichMedia,
                                       base::JSON_PARSE_CHROMIUM_EXTENSIONS));
}

TEST_F(
    NTPBackgroundImagesServiceTest,
    StaleCallbackFromPreviousComponentsAreDiscardedWhenCountryChangesBackToOriginal) {
  Init();

  service_->SetSponsoredComponentJsonForTesting(GetComponentId("US"),
                                                kTestSponsoredImages);
  service_->SetSponsoredComponentJsonForTesting(GetComponentId("GB"),
                                                kTestRichMedia);

  service_->SetCountryCode("US");
  service_->RegisterSponsoredImagesComponent();

  service_->SetCountryCode("GB");
  service_->RegisterSponsoredImagesComponent();

  service_->SetCountryCode("US");
  service_->RegisterSponsoredImagesComponent();
  ASSERT_TRUE(base::test::RunUntil([this]() {
    return service_->on_handled_sponsored_component_data_called();
  }));

  EXPECT_TRUE(service_->GetSponsoredImagesData(/*supports_rich_media=*/true));
  EXPECT_TRUE(observer_.on_sponsored_images_updated);
  EXPECT_TRUE(observer_.sponsored_images_data);
  EXPECT_EQ(observer_.sponsored_content_data(),
            base::JSONReader::ReadDict(kTestSponsoredImages,
                                       base::JSON_PARSE_CHROMIUM_EXTENSIONS));
}

}  // namespace ntp_background_images
