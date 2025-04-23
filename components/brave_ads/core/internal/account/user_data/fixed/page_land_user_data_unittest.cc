/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/page_land_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPageLandUserDataTest : public test::TestBase {};

TEST_F(BraveAdsPageLandUserDataTest,
       BuildPageLandUserDataForHttpInformationalResponseStatusCodeClass) {
  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"JSON(
                    {
                      "httpResponseStatus": "1xx"
                    })JSON"),
            BuildPageLandUserData(net::HTTP_SWITCHING_PROTOCOLS));
}

TEST_F(BraveAdsPageLandUserDataTest,
       BuildPageLandUserDataForHttpSuccessfulResponseStatusCodeClass) {
  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"JSON(
                    {
                      "httpResponseStatus": "2xx"
                    })JSON"),
            BuildPageLandUserData(net::HTTP_IM_USED));
}

TEST_F(BraveAdsPageLandUserDataTest,
       BuildPageLandUserDataForHttpRedirectionMessageStatusCodeClass) {
  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"JSON(
                    {
                      "httpResponseStatus": "3xx"
                    })JSON"),
            BuildPageLandUserData(net::HTTP_MOVED_PERMANENTLY));
}

TEST_F(BraveAdsPageLandUserDataTest,
       BuildPageLandUserDataForHttpClientErrorResponseStatusCode) {
  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"JSON(
                    {
                      "httpResponseStatus": "404"
                    })JSON"),
            BuildPageLandUserData(net::HTTP_NOT_FOUND));
}

TEST_F(BraveAdsPageLandUserDataTest,
       BuildPageLandUserDataForHttpClientErrorResponseStatusCodeClass) {
  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"JSON(
                    {
                      "httpResponseStatus": "4xx"
                    })JSON"),
            BuildPageLandUserData(net::HTTP_UPGRADE_REQUIRED));
}

TEST_F(
    BraveAdsPageLandUserDataTest,
    BuildPageLandUserDataForPrivacyPreservingHttpServerErrorResponseStatusCode) {
  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"JSON(
                    {
                      "httpResponseStatus": "500"
                    })JSON"),
            BuildPageLandUserData(net::HTTP_INTERNAL_SERVER_ERROR));
}

TEST_F(
    BraveAdsPageLandUserDataTest,
    BuildPageLandUserDataForPrivacyPreservingHttpServerErrorResponseStatusCodeClass) {
  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"JSON(
                    {
                      "httpResponseStatus": "5xx"
                    })JSON"),
            BuildPageLandUserData(net::HTTP_LOOP_DETECTED));
}

TEST_F(
    BraveAdsPageLandUserDataTest,
    DoNotBuildPageLandUserDataForHttpResponseStatusErrorPageForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_THAT(BuildPageLandUserData(net::HTTP_NOT_FOUND), ::testing::IsEmpty());
}

}  // namespace brave_ads
