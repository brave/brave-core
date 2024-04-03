/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/create_reward_confirmation_url_request_builder.h"

#include <optional>

#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_build_channel_types.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_converter_util.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreateRewardConfirmationUrlRequestBuilderTest
    : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    MockConfirmationUserData();

    AdvanceClockTo(TimeFromUTCString("Mon, 8 Jul 1996 09:25"));
  }

  TokenGeneratorMock token_generator_mock_;
};

TEST_F(BraveAdsCreateRewardConfirmationUrlRequestBuilderTest,
       BuildUrlForLargeAnonmityCountry) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(&token_generator_mock_,
                                    /*should_use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  CreateRewardConfirmationUrlRequestBuilder url_request_builder(*confirmation);

  // Act
  mojom::UrlRequestInfoPtr const url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestInfoPtr expected_url_request = mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(
      R"(https://anonymous.ads.bravesoftware.com/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/eyJzaWduYXR1cmUiOiJjZjBLajJpbVB6d0ZOYk9NMEtnSGYxNVkxRmRNY1cxaS90R2hwbVVWbUhqK25KbXphNko1NkkrUmpVd3A3alJOZ3lMR2lQcTUwZm1mZ1Y5WEorTDBOUT09IiwidCI6IlBMb3d6MldGMmVHRDV6Zndaams5cDc2SFhCTERLTXEvM0VBWkhlRy9mRTJYR1E0OGp5dGUrVmU1MFpsYXNPdVlMNW13QThDVTJhRk1sSnJ0M0REZ0N3PT0ifQ==)");
  expected_url_request->headers = {"accept: application/json"};
  expected_url_request->content =
      R"({"blindedPaymentTokens":["Ev5JE4/9TZI/5TqyN9JWfJ1To0HBwQw2rWeAPcdjX3Q="],"buildChannel":"release","catalog":[{"id":"29e5c8bc0ba319069980bb390d8e8f9b58c05a20"}],"countryCode":"US","createdAtTimestamp":"1996-07-08T09:00:00.000Z","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","diagnosticId":"c1298fde-7fdb-401f-a3ce-0b58fe86e6e2","platform":"windows","publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=","rotating_hash":"jBdiJH7Hu3wj31WWNLjKV5nVxFxWSDWkYh5zXCS3rXY=","segment":"untargeted","studies":[],"systemTimestamp":"1996-07-08T09:00:00.000Z","topSegment":[],"transactionId":"8b742869-6e4a-490c-ac31-31b49130098a","type":"view","versionNumber":"1.2.3.4"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethodType::kPost;
  EXPECT_EQ(expected_url_request, url_request);
}

TEST_F(BraveAdsCreateRewardConfirmationUrlRequestBuilderTest,
       BuildUrlForAnonymousCountry) {
  // Arrange
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"en_AS"};

  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(&token_generator_mock_,
                                    /*should_use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  CreateRewardConfirmationUrlRequestBuilder url_request_builder(*confirmation);

  // Act
  mojom::UrlRequestInfoPtr const url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestInfoPtr expected_url_request = mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(
      R"(https://anonymous.ads.bravesoftware.com/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/eyJzaWduYXR1cmUiOiJrcnRqRWlCT1NyN1ByVnNpdlJjU1VyWUlneVp5d3dRcWZsR2dwWXE0NmNQZEVaRnZvTk5uRlJFNklMMUhXdHJvVGdDc0ZFRFZqeW05V1I2SWc1OUd0UT09IiwidCI6IlBMb3d6MldGMmVHRDV6Zndaams5cDc2SFhCTERLTXEvM0VBWkhlRy9mRTJYR1E0OGp5dGUrVmU1MFpsYXNPdVlMNW13QThDVTJhRk1sSnJ0M0REZ0N3PT0ifQ==)");
  expected_url_request->headers = {"accept: application/json"};
  expected_url_request->content =
      R"({"blindedPaymentTokens":["Ev5JE4/9TZI/5TqyN9JWfJ1To0HBwQw2rWeAPcdjX3Q="],"buildChannel":"release","catalog":[{"id":"29e5c8bc0ba319069980bb390d8e8f9b58c05a20"}],"countryCode":"??","createdAtTimestamp":"1996-07-08T09:00:00.000Z","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","diagnosticId":"c1298fde-7fdb-401f-a3ce-0b58fe86e6e2","platform":"windows","publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=","rotating_hash":"jBdiJH7Hu3wj31WWNLjKV5nVxFxWSDWkYh5zXCS3rXY=","segment":"untargeted","studies":[],"systemTimestamp":"1996-07-08T09:00:00.000Z","topSegment":[],"transactionId":"8b742869-6e4a-490c-ac31-31b49130098a","type":"view","versionNumber":"1.2.3.4"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethodType::kPost;
  EXPECT_EQ(expected_url_request, url_request);
}

TEST_F(BraveAdsCreateRewardConfirmationUrlRequestBuilderTest,
       BuildUrlForOtherCountry) {
  // Arrange
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"en_KY"};

  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(&token_generator_mock_,
                                    /*should_use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  CreateRewardConfirmationUrlRequestBuilder url_request_builder(*confirmation);

  // Act
  mojom::UrlRequestInfoPtr const url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestInfoPtr expected_url_request = mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(
      R"(https://anonymous.ads.bravesoftware.com/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/eyJzaWduYXR1cmUiOiJLUFF2NmI5QnpjdFVjcWEvRlA2OTk4WXJDZzVDZHJzSUZiQ01iZ3E4Qy9JTTdGZk0rR2xxVzFrOHVTcUlKTTB2VXg5TnQ4a1MrZTdOdllsbFM4M0lWUT09IiwidCI6IlBMb3d6MldGMmVHRDV6Zndaams5cDc2SFhCTERLTXEvM0VBWkhlRy9mRTJYR1E0OGp5dGUrVmU1MFpsYXNPdVlMNW13QThDVTJhRk1sSnJ0M0REZ0N3PT0ifQ==)");
  expected_url_request->headers = {"accept: application/json"};
  expected_url_request->content =
      R"({"blindedPaymentTokens":["Ev5JE4/9TZI/5TqyN9JWfJ1To0HBwQw2rWeAPcdjX3Q="],"buildChannel":"release","catalog":[{"id":"29e5c8bc0ba319069980bb390d8e8f9b58c05a20"}],"createdAtTimestamp":"1996-07-08T09:00:00.000Z","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","diagnosticId":"c1298fde-7fdb-401f-a3ce-0b58fe86e6e2","platform":"windows","publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=","rotating_hash":"jBdiJH7Hu3wj31WWNLjKV5nVxFxWSDWkYh5zXCS3rXY=","segment":"untargeted","studies":[],"systemTimestamp":"1996-07-08T09:00:00.000Z","topSegment":[],"transactionId":"8b742869-6e4a-490c-ac31-31b49130098a","type":"view","versionNumber":"1.2.3.4"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethodType::kPost;
  EXPECT_EQ(expected_url_request, url_request);
}

TEST_F(BraveAdsCreateRewardConfirmationUrlRequestBuilderTest,
       BuildUrlForLargeAnonmityCountryAndNonReleaseBuildChannel) {
  // Arrange
  MockBuildChannel(BuildChannelType::kNightly);

  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(&token_generator_mock_,
                                    /*should_use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  CreateRewardConfirmationUrlRequestBuilder url_request_builder(*confirmation);

  // Act
  mojom::UrlRequestInfoPtr const url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestInfoPtr expected_url_request = mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(
      R"(https://anonymous.ads.bravesoftware.com/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/eyJzaWduYXR1cmUiOiJaT01rYVpOR01QZjFCZkd3YWJxYzViUGJFM2NYQ0QvV0FzR3BJYzRJQkxUSXdiWkZscnlYZEJxSDFtRklZbXVNSGhJempCVXNBVUNUSGIvZFJaUnZYQT09IiwidCI6IlBMb3d6MldGMmVHRDV6Zndaams5cDc2SFhCTERLTXEvM0VBWkhlRy9mRTJYR1E0OGp5dGUrVmU1MFpsYXNPdVlMNW13QThDVTJhRk1sSnJ0M0REZ0N3PT0ifQ==)");
  expected_url_request->headers = {"accept: application/json"};
  expected_url_request->content =
      R"({"blindedPaymentTokens":["Ev5JE4/9TZI/5TqyN9JWfJ1To0HBwQw2rWeAPcdjX3Q="],"buildChannel":"nightly","catalog":[{"id":"29e5c8bc0ba319069980bb390d8e8f9b58c05a20"}],"createdAtTimestamp":"1996-07-08T09:00:00.000Z","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","diagnosticId":"c1298fde-7fdb-401f-a3ce-0b58fe86e6e2","platform":"windows","publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=","rotating_hash":"jBdiJH7Hu3wj31WWNLjKV5nVxFxWSDWkYh5zXCS3rXY=","segment":"untargeted","studies":[],"systemTimestamp":"1996-07-08T09:00:00.000Z","topSegment":[],"transactionId":"8b742869-6e4a-490c-ac31-31b49130098a","type":"view","versionNumber":"1.2.3.4"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethodType::kPost;
  EXPECT_EQ(expected_url_request, url_request);
}

TEST_F(BraveAdsCreateRewardConfirmationUrlRequestBuilderTest,
       BuildUrlForAnonymousCountryAndNonReleaseBuildChannel) {
  // Arrange
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"en_AS"};

  MockBuildChannel(BuildChannelType::kNightly);

  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(&token_generator_mock_,
                                    /*should_use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  CreateRewardConfirmationUrlRequestBuilder url_request_builder(*confirmation);

  // Act
  mojom::UrlRequestInfoPtr const url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestInfoPtr expected_url_request = mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(
      R"(https://anonymous.ads.bravesoftware.com/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/eyJzaWduYXR1cmUiOiJaT01rYVpOR01QZjFCZkd3YWJxYzViUGJFM2NYQ0QvV0FzR3BJYzRJQkxUSXdiWkZscnlYZEJxSDFtRklZbXVNSGhJempCVXNBVUNUSGIvZFJaUnZYQT09IiwidCI6IlBMb3d6MldGMmVHRDV6Zndaams5cDc2SFhCTERLTXEvM0VBWkhlRy9mRTJYR1E0OGp5dGUrVmU1MFpsYXNPdVlMNW13QThDVTJhRk1sSnJ0M0REZ0N3PT0ifQ==)");
  expected_url_request->headers = {"accept: application/json"};
  expected_url_request->content =
      R"({"blindedPaymentTokens":["Ev5JE4/9TZI/5TqyN9JWfJ1To0HBwQw2rWeAPcdjX3Q="],"buildChannel":"nightly","catalog":[{"id":"29e5c8bc0ba319069980bb390d8e8f9b58c05a20"}],"createdAtTimestamp":"1996-07-08T09:00:00.000Z","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","diagnosticId":"c1298fde-7fdb-401f-a3ce-0b58fe86e6e2","platform":"windows","publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=","rotating_hash":"jBdiJH7Hu3wj31WWNLjKV5nVxFxWSDWkYh5zXCS3rXY=","segment":"untargeted","studies":[],"systemTimestamp":"1996-07-08T09:00:00.000Z","topSegment":[],"transactionId":"8b742869-6e4a-490c-ac31-31b49130098a","type":"view","versionNumber":"1.2.3.4"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethodType::kPost;
  EXPECT_EQ(expected_url_request, url_request);
}

TEST_F(BraveAdsCreateRewardConfirmationUrlRequestBuilderTest,
       BuildUrlForOtherCountryAndNonReleaseBuildChannel) {
  // Arrange
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"en_KY"};

  MockBuildChannel(BuildChannelType::kNightly);

  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(&token_generator_mock_,
                                    /*should_use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  CreateRewardConfirmationUrlRequestBuilder url_request_builder(*confirmation);

  // Act
  mojom::UrlRequestInfoPtr const url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestInfoPtr expected_url_request = mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(
      R"(https://anonymous.ads.bravesoftware.com/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/eyJzaWduYXR1cmUiOiJaT01rYVpOR01QZjFCZkd3YWJxYzViUGJFM2NYQ0QvV0FzR3BJYzRJQkxUSXdiWkZscnlYZEJxSDFtRklZbXVNSGhJempCVXNBVUNUSGIvZFJaUnZYQT09IiwidCI6IlBMb3d6MldGMmVHRDV6Zndaams5cDc2SFhCTERLTXEvM0VBWkhlRy9mRTJYR1E0OGp5dGUrVmU1MFpsYXNPdVlMNW13QThDVTJhRk1sSnJ0M0REZ0N3PT0ifQ==)");
  expected_url_request->headers = {"accept: application/json"};
  expected_url_request->content =
      R"({"blindedPaymentTokens":["Ev5JE4/9TZI/5TqyN9JWfJ1To0HBwQw2rWeAPcdjX3Q="],"buildChannel":"nightly","catalog":[{"id":"29e5c8bc0ba319069980bb390d8e8f9b58c05a20"}],"createdAtTimestamp":"1996-07-08T09:00:00.000Z","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","diagnosticId":"c1298fde-7fdb-401f-a3ce-0b58fe86e6e2","platform":"windows","publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=","rotating_hash":"jBdiJH7Hu3wj31WWNLjKV5nVxFxWSDWkYh5zXCS3rXY=","segment":"untargeted","studies":[],"systemTimestamp":"1996-07-08T09:00:00.000Z","topSegment":[],"transactionId":"8b742869-6e4a-490c-ac31-31b49130098a","type":"view","versionNumber":"1.2.3.4"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethodType::kPost;
  EXPECT_EQ(expected_url_request, url_request);
}

}  // namespace brave_ads
