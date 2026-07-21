/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_site_image_source.h"

#include <memory>

#include "base/files/file_path.h"
#include "base/test/scoped_command_line.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service_waiter.h"
#include "brave/components/ntp_background_images/browser/switches.h"
#include "brave/components/ntp_background_images/browser/test/ntp_sponsored_source_test_util.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ntp_background_images {

namespace {

base::FilePath GetComponentPath() {
  return test::GetSponsoredImagesComponentPath().AppendASCII("image");
}

}  // namespace

class NTPSponsoredSiteImageSourceTest : public testing::Test {
 protected:
  void SetUp() override {
    NTPBackgroundImagesService::RegisterLocalStatePrefsForMigration(
        pref_service_.registry());

    background_images_service_ = std::make_unique<NTPBackgroundImagesService>(
        /*variations_service=*/nullptr, /*component_update_service=*/nullptr,
        &pref_service_);
    url_data_source_ = std::make_unique<NTPSponsoredSiteImageSource>(
        background_images_service_.get());

    scoped_command_line_.GetProcessCommandLine()->AppendSwitchPath(
        switches::kOverrideSponsoredImagesComponentPath, GetComponentPath());

    NTPBackgroundImagesServiceWaiter waiter(*background_images_service_);
    background_images_service_->Init();
    waiter.WaitForOnSponsoredSitesDataDidUpdate();
  }

  NTPSponsoredSiteImageSource* url_data_source() const {
    return url_data_source_.get();
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedCommandLine scoped_command_line_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<NTPBackgroundImagesService> background_images_service_;
  std::unique_ptr<NTPSponsoredSiteImageSource> url_data_source_;
};

TEST_F(NTPSponsoredSiteImageSourceTest, StartDataRequest) {
  EXPECT_THAT(test::StartDataRequest(
                  url_data_source(),
                  GURL(R"(chrome://sponsored-site-image/tiles/image_1.webp)")),
              ::testing::Not(::testing::IsEmpty()));
  EXPECT_THAT(test::StartDataRequest(
                  url_data_source(),
                  GURL(R"(chrome://sponsored-site-image/tiles/image_2.webp)")),
              ::testing::Not(::testing::IsEmpty()));
}

TEST_F(NTPSponsoredSiteImageSourceTest,
       DoNotStartDataRequestForMissingSponsoredSiteImage) {
  EXPECT_THAT(test::StartDataRequest(
                  url_data_source(),
                  GURL(R"(chrome://sponsored-site-image/tiles/unknown.webp)")),
              ::testing::IsEmpty());
}

TEST_F(NTPSponsoredSiteImageSourceTest,
       DoNotStartDataRequestIfContentIsReferencingParentDirectory) {
  EXPECT_THAT(
      test::StartDataRequest(
          url_data_source(),
          GURL(R"(chrome://sponsored-site-image/tiles/../restricted.jpg)")),
      ::testing::IsEmpty());
  EXPECT_THAT(test::StartDataRequest(
                  url_data_source(),
                  GURL(R"(chrome://sponsored-site-image/restricted.jpg)")),
              ::testing::IsEmpty());
}

TEST_F(NTPSponsoredSiteImageSourceTest, GetMimeType) {
  EXPECT_EQ("image/webp",
            url_data_source()->GetMimeType(
                GURL(R"(chrome://sponsored-site-image/tiles/image.webp)")));
}

TEST_F(NTPSponsoredSiteImageSourceTest, AllowCaching) {
  EXPECT_FALSE(url_data_source()->AllowCaching());
}

}  // namespace ntp_background_images
