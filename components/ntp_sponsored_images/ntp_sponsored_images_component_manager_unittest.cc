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
  NTPSponsoredImagesComponentManager manager(nullptr, nullptr);

  // By default manager doesn't have data.
  EXPECT_FALSE(manager.GetLatestSponsoredImagesData());
}

TEST(NTPSponsoredImagesComponentManagerTest, InternalDataTest) {
  TestObserver observer;
  NTPSponsoredImagesComponentManager manager(nullptr, nullptr);
  manager.AddObserver(&observer);

  // Check with json file with empty object.
  manager.ResetInternalImagesDataForTest();
  manager.OnGetPhotoJsonData("{}");
  EXPECT_TRUE(manager.GetLatestSponsoredImagesData());
  manager.NotifyObservers();
  EXPECT_TRUE(observer.called_);
  EXPECT_TRUE(observer.data_.logo_alt_text.empty());

  const std::string  test_json_string = R"(
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
  manager.ResetInternalImagesDataForTest();
  manager.OnGetPhotoJsonData(test_json_string);
  auto images_data = manager.GetLatestSponsoredImagesData();
  EXPECT_TRUE(images_data);
  // Above json data has 3 wallpapers.
  EXPECT_EQ(3, images_data->wallpaper_image_count);
  observer.called_ = false;
  manager.NotifyObservers();
  EXPECT_TRUE(observer.called_);
  EXPECT_FALSE(observer.data_.logo_alt_text.empty());

  manager.RemoveObserver(&observer);
}
