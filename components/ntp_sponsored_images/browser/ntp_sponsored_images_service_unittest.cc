/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_internal_data.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_service.h"
#include "testing/gtest/include/gtest/gtest.h"

class TestObserver : public NTPSponsoredImagesService::Observer {
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

TEST(NTPSponsoredImagesServiceTest, BasicTest) {
  NTPSponsoredImagesService service(nullptr);

  // By default manager doesn't have data.
  EXPECT_FALSE(service.GetLatestSponsoredImagesData());
}

TEST(NTPSponsoredImagesServiceTest, InternalDataTest) {
  TestObserver observer;
  NTPSponsoredImagesService service(nullptr);
  service.AddObserver(&observer);

  // Check with json file with empty object.
  service.ResetInternalImagesDataForTest();
  service.OnGetPhotoJsonData("{}");
  auto data = service.GetLatestSponsoredImagesData();
  EXPECT_TRUE(data);
  EXPECT_FALSE(data->IsValid());
  service.NotifyObservers();
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
  service.ResetInternalImagesDataForTest();
  service.OnGetPhotoJsonData(test_json_string);
  data = service.GetLatestSponsoredImagesData();
  EXPECT_TRUE(data);
  EXPECT_TRUE(data->IsValid());
  // Above json data has 3 wallpapers.
  const size_t image_count = 3;
  EXPECT_EQ(image_count, data->wallpaper_image_urls.size());
  observer.called_ = false;
  service.NotifyObservers();
  EXPECT_TRUE(observer.called_);
  EXPECT_FALSE(observer.data_.logo_alt_text.empty());

  service.RemoveObserver(&observer);
}
