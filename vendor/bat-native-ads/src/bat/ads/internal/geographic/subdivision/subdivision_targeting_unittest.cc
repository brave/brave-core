/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"

#include <memory>

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsSubdivisionTargetingTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    subdivision_targeting_ =
        std::make_unique<geographic::SubdivisionTargeting>();
  }

  std::unique_ptr<geographic::SubdivisionTargeting> subdivision_targeting_;
};

TEST_F(BatAdsSubdivisionTargetingTest,
       AutoDetectSubdivisionTargetingAllowedRegion) {
  // Arrange
  const URLResponseMap url_responses = {
      {"/v1/getstate", {{net::HTTP_OK, R"({"country":"US", "region":"AL"})"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  subdivision_targeting_->MaybeFetch();

  // Assert
  EXPECT_TRUE(geographic::SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_TRUE(subdivision_targeting_->ShouldAutoDetect());

  EXPECT_EQ("US-AL", AdsClientHelper::GetInstance()->GetStringPref(
                         prefs::kAutoDetectedSubdivisionTargetingCode));
}

TEST_F(BatAdsSubdivisionTargetingTest, AutoDetectSubdivisionTargetingNoRegion) {
  // Arrange
  const URLResponseMap url_responses = {
      {"/v1/getstate",
       {{net::HTTP_OK, R"({"country":"US", "region":"NO REGION"})"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  subdivision_targeting_->MaybeFetch();

  // Assert
  EXPECT_TRUE(geographic::SubdivisionTargeting::ShouldAllow());
  EXPECT_TRUE(subdivision_targeting_->IsDisabled());
  EXPECT_FALSE(subdivision_targeting_->ShouldAutoDetect());
}

TEST_F(BatAdsSubdivisionTargetingTest,
       AutoDetectSubdivisionTargetingWrongRegion) {
  // Arrange
  const URLResponseMap url_responses = {
      {"/v1/getstate", {{net::HTTP_OK, R"({"country":"ES", "region":"AN"})"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  subdivision_targeting_->MaybeFetch();

  // Assert
  EXPECT_FALSE(geographic::SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_TRUE(subdivision_targeting_->ShouldAutoDetect());
}

TEST_F(BatAdsSubdivisionTargetingTest,
       MaybeFetchSubdivisionTargetingNotSupportedLocale) {
  // Arrange
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"en_KY"};

  // Act
  subdivision_targeting_->MaybeFetch();

  // Assert
  EXPECT_FALSE(geographic::SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_TRUE(subdivision_targeting_->ShouldAutoDetect());
}

TEST_F(BatAdsSubdivisionTargetingTest,
       MaybeAllowSubdivisionTargetingNotSupportedLocale) {
  // Arrange
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"en_KY"};

  // Act
  subdivision_targeting_->MaybeAllow();

  // Assert
  EXPECT_FALSE(geographic::SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_TRUE(subdivision_targeting_->ShouldAutoDetect());
}

TEST_F(BatAdsSubdivisionTargetingTest,
       MaybeAllowSubdivisionTargetingWrongRegion) {
  // Arrange
  AdsClientHelper::GetInstance()->SetStringPref(
      prefs::kSubdivisionTargetingCode, "CA-QC");

  // Act
  subdivision_targeting_->MaybeAllow();

  // Assert
  EXPECT_FALSE(geographic::SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_TRUE(subdivision_targeting_->ShouldAutoDetect());
}

class BatAdsSubdivisionTargetingRetryOnInvalidResponseTest
    : public BatAdsSubdivisionTargetingTest,
      public ::testing::WithParamInterface<const char*> {};

TEST_P(BatAdsSubdivisionTargetingRetryOnInvalidResponseTest,
       FetchSubdivisionTargetingRetryOnInvalidResponse) {
  // Arrange
  const URLResponseMap url_responses = {
      {"/v1/getstate",
       {{net::HTTP_OK, GetParam()},
        {net::HTTP_OK, R"({"country":"US", "region":"AL"})"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  subdivision_targeting_->MaybeFetch();
  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_TRUE(geographic::SubdivisionTargeting::ShouldAllow());
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
