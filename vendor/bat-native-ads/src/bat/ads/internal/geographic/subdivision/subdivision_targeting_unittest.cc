/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"

#include <memory>

#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_mock_util.h"
#include "bat/ads/pref_names.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsSubdivisionTargetingTest : public UnitTestBase {
 protected:
  BatAdsSubdivisionTargetingTest() = default;

  void SetUp() override {
    UnitTestBase::SetUp();

    subdivision_targeting_ =
        std::make_unique<geographic::SubdivisionTargeting>();
  }

  void SetUpMocks() override { MockLocaleHelper(locale_helper_mock_, "en-US"); }

  std::unique_ptr<geographic::SubdivisionTargeting> subdivision_targeting_;
};

TEST_F(BatAdsSubdivisionTargetingTest,
       AutoDetectSubdivisionTargetingAllowedRegion) {
  // Arrange
  const URLResponseMap url_responses = {
      {R"(/v1/getstate)",
       {{net::HTTP_OK, R"({"country":"US", "region":"AL"})"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  subdivision_targeting_->MaybeFetch();

  // Assert
  EXPECT_TRUE(subdivision_targeting_->ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());

  EXPECT_EQ("US-AL", AdsClientHelper::GetInstance()->GetStringPref(
                         prefs::kAutoDetectedSubdivisionTargetingCode));
}

TEST_F(BatAdsSubdivisionTargetingTest, AutoDetectSubdivisionTargetingNoRegion) {
  // Arrange
  const URLResponseMap url_responses = {
      {R"(/v1/getstate)",
       {{net::HTTP_OK, R"({"country":"US", "region":"NO REGION"})"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  subdivision_targeting_->MaybeFetch();

  // Assert
  EXPECT_TRUE(subdivision_targeting_->ShouldAllow());
  EXPECT_TRUE(subdivision_targeting_->IsDisabled());
}

TEST_F(BatAdsSubdivisionTargetingTest,
       MaybeFetchSubdivisionTargetingNotSupportedLocale) {
  // Arrange
  MockLocaleHelper(locale_helper_mock_, "en-KY");

  // Act
  subdivision_targeting_->MaybeFetch();

  // Assert
  EXPECT_FALSE(subdivision_targeting_->ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
}

TEST_F(BatAdsSubdivisionTargetingTest,
       MaybeALLOWSubdivisionTargetingNotSupportedLocale) {
  // Arrange
  MockLocaleHelper(locale_helper_mock_, "en-KY");

  // Act
  subdivision_targeting_->MaybeAllow();

  // Assert
  EXPECT_FALSE(subdivision_targeting_->ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
}

TEST_F(BatAdsSubdivisionTargetingTest,
       MaybeAllowSubdivisionTargetingWrongRegion) {
  // Arrange
  AdsClientHelper::GetInstance()->SetStringPref(
      prefs::kSubdivisionTargetingCode, "CA-QC");

  // Act
  subdivision_targeting_->MaybeAllow();

  // Assert
  EXPECT_TRUE(subdivision_targeting_->ShouldAllow());
  EXPECT_TRUE(subdivision_targeting_->IsDisabled());
}

class BatAdsSubdivisionTargetingRetryOnInvalidResponseTest
    : public BatAdsSubdivisionTargetingTest,
      public ::testing::WithParamInterface<const char*> {};

TEST_P(BatAdsSubdivisionTargetingRetryOnInvalidResponseTest,
       FetchSubdivisionTargetingRetryOnInvalidResponse) {
  // Arrange
  const URLResponseMap url_responses = {
      {R"(/v1/getstate)",
       {{net::HTTP_OK, GetParam()},
        {net::HTTP_OK, R"({"country":"US", "region":"AL"})"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  subdivision_targeting_->MaybeFetch();
  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_TRUE(subdivision_targeting_->ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());

  EXPECT_EQ("US-AL", AdsClientHelper::GetInstance()->GetStringPref(
                         prefs::kAutoDetectedSubdivisionTargetingCode));
}

INSTANTIATE_TEST_SUITE_P(,
                         BatAdsSubdivisionTargetingRetryOnInvalidResponseTest,
                         ::testing::Values(R"("invalid_json")",
                                           R"({"country":"US"})",
                                           R"({"region":"CA"})",
                                           R"({"country":"", "region":"CA"})",
                                           R"({"country":"US", "region":""})"));

}  // namespace ads
