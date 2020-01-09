/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/ntp_sponsored_images/ntp_sponsored_images_component_manager.h"
#include "brave/components/ntp_sponsored_images/ntp_sponsored_images_data.h"
#include "brave/components/ntp_sponsored_images/ntp_sponsored_images_internal_data.h"
#include "testing/gtest/include/gtest/gtest.h"

class TestObserver : public NTPSponsoredImagesComponentManager::Observer {
 public:
  TestObserver() = default;
  ~TestObserver() override = default;

  void OnUpdated(const NTPSponsoredImagesData& data) override {
    called_ = true;
    data_ = data;
  }

  NTPSponsoredImagesData data_;
  bool called_ = false;
};

TEST(NTPSponsoredImagesComponentManagerTest, BasicTest) {
  NTPSponsoredImagesComponentManager manager(nullptr, nullptr, std::string());

  // By default manager doesn't have data.
  EXPECT_FALSE(manager.GetLatestSponsoredImagesData());
  EXPECT_FALSE(
      manager.IsValidImage(NTPSponsoredImageSource::Type::TYPE_LOGO, 0));
  EXPECT_FALSE(
      manager.IsValidImage(NTPSponsoredImageSource::Type::TYPE_WALLPAPER, 0));
}

TEST(NTPSponsoredImagesComponentManagerTest, InternalDataTest) {
  TestObserver observer;
  NTPSponsoredImagesComponentManager manager(nullptr, nullptr, std::string());
  manager.AddObserver(&observer);

  manager.ResetInternalImagesDataForTest();
  manager.OnGetPhotoJsonData("");
  EXPECT_FALSE(manager.GetLatestSponsoredImagesData());
  manager.NotifyObservers();
  EXPECT_TRUE(observer.called_);
  EXPECT_TRUE(observer.data_.logo_alt_text.empty());

  const std::string  test_json_string =
      "{"
          "\"logoImageUrl\":  \"logo.png\","
          "\"logoAltText\": \"Technikke: For music lovers\","
          "\"logoDestinationUrl\": \"https://www.brave.com/\","
          "\"logoCompanyName\": \"Technikke\","
          "\"wallpaperImageUrls\": ["
              "\"background-1.jpg\","
              "\"background-2.jpg\","
              "\"background-3.jpg\""
         "]"
     "}";
  manager.ResetInternalImagesDataForTest();
  manager.OnGetPhotoJsonData(test_json_string);
  // Only wall paper at index 3 failed because json has only 3 wallpaers.
  EXPECT_TRUE(manager.GetLatestSponsoredImagesData());
  EXPECT_TRUE(
      manager.IsValidImage(NTPSponsoredImageSource::Type::TYPE_LOGO, 0));
  EXPECT_TRUE(
      manager.IsValidImage(NTPSponsoredImageSource::Type::TYPE_WALLPAPER, 0));
  EXPECT_TRUE(
      manager.IsValidImage(NTPSponsoredImageSource::Type::TYPE_WALLPAPER, 1));
  EXPECT_TRUE(
      manager.IsValidImage(NTPSponsoredImageSource::Type::TYPE_WALLPAPER, 2));
  EXPECT_FALSE(
      manager.IsValidImage(NTPSponsoredImageSource::Type::TYPE_WALLPAPER, 3));
  observer.called_ = false;
  manager.NotifyObservers();
  EXPECT_TRUE(observer.called_);
  EXPECT_FALSE(observer.data_.logo_alt_text.empty());

  manager.RemoveObserver(&observer);
}
