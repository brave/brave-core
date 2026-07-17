/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/test/ntp_sponsored_site_image_source_test_base.h"
#include "brave/components/ntp_background_images/browser/test/ntp_sponsored_source_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ntp_background_images {

namespace {

base::FilePath GetComponentPath() {
  return test::GetSponsoredImagesComponentPath().AppendASCII("image");
}

}  // namespace

class NTPSponsoredSiteImageSourceTest
    : public test::NTPSponsoredSiteImageSourceTestBase {
 protected:
  void SetUp() override {
    test::NTPSponsoredSiteImageSourceTestBase::SetUp();
    SimulateOnSponsoredSitesDataDidUpdate(GetComponentPath());
  }
};

TEST_F(NTPSponsoredSiteImageSourceTest, StartDataRequest) {
  EXPECT_THAT(test::StartDataRequest(
                  url_data_source(),
                  GURL(R"(chrome://sponsored-site-image/tiles/image_1.png)")),
              ::testing::Not(::testing::IsEmpty()));
  EXPECT_THAT(test::StartDataRequest(
                  url_data_source(),
                  GURL(R"(chrome://sponsored-site-image/tiles/image_2.png)")),
              ::testing::Not(::testing::IsEmpty()));
}

TEST_F(NTPSponsoredSiteImageSourceTest,
       DoNotStartDataRequestForMissingSponsoredSiteImage) {
  EXPECT_THAT(test::StartDataRequest(
                  url_data_source(),
                  GURL(R"(chrome://sponsored-site-image/tiles/unknown.png)")),
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
  EXPECT_EQ("image/png",
            url_data_source()->GetMimeType(
                GURL(R"(chrome://sponsored-site-image/tiles/image.png)")));
}

TEST_F(NTPSponsoredSiteImageSourceTest, AllowCaching) {
  EXPECT_FALSE(url_data_source()->AllowCaching());
}

}  // namespace ntp_background_images
