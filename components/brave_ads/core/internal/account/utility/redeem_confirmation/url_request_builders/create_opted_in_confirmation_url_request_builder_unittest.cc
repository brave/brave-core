/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/url_request_builders/create_opted_in_confirmation_url_request_builder.h"

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_build_channel_types.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

namespace {

constexpr char kExpectedUrl[] =
    "https://anonymous.ads.bravesoftware.com/v3/confirmation/"
    "8b742869-6e4a-490c-ac31-31b49130098a/"
    "eyJzaWduYXR1cmUiOiJrM3hJalZwc0FYTGNHL0NKRGVLQVphN0g3aGlrMVpyUThIOVpEZC9KVU"
    "1SQWdtYk5WY0V6VnhRb2dDZDBjcmlDZnZCQWtsd1hybWNyeVBaaFUxMlg3Zz09IiwidCI6IlBM"
    "b3d6MldGMmVHRDV6Zndaams5cDc2SFhCTERLTXEvM0VBWkhlRy9mRTJYR1E0OGp5dGUrVmU1MF"
    "psYXNPdVlMNW13QThDVTJhRk1sSnJ0M0REZ0N3PT0ifQ==";
constexpr char kExpectedContent[] =
    R"({"blindedPaymentTokens":["Ev5JE4/9TZI/5TqyN9JWfJ1To0HBwQw2rWeAPcdjX3Q="],"creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=","transactionId":"8b742869-6e4a-490c-ac31-31b49130098a","type":"view"})";

}  // namespace

class BatAdsCreateOptedInConfirmationUrlRequestBuilderTest
    : public UnitTestBase {};

TEST_F(BatAdsCreateOptedInConfirmationUrlRequestBuilderTest,
       BuildUrlForLargeAnonmityCountry) {
  // Arrange
  GlobalState::GetInstance()->Flags().environment_type =
      mojom::EnvironmentType::kStaging;

  privacy::SetUnblindedTokens(/*count*/ 1);

  MockBuildChannel(BuildChannelType::kRelease);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);
  CreateOptedInConfirmationUrlRequestBuilder url_request_builder(*confirmation);

  // Act
  mojom::UrlRequestInfoPtr const url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestInfoPtr expected_url_request = mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(kExpectedUrl);
  expected_url_request->headers = {"accept: application/json"};
  expected_url_request->content = kExpectedContent;
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethodType::kPost;

  EXPECT_EQ(url_request, expected_url_request);
}

TEST_F(BatAdsCreateOptedInConfirmationUrlRequestBuilderTest,
       BuildUrlForAnonymousCountry) {
  // Arrange
  GlobalState::GetInstance()->Flags().environment_type =
      mojom::EnvironmentType::kStaging;

  privacy::SetUnblindedTokens(/*count*/ 1);

  MockBuildChannel(BuildChannelType::kRelease);

  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"en_AS"};

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);
  CreateOptedInConfirmationUrlRequestBuilder url_request_builder(*confirmation);

  // Act
  mojom::UrlRequestInfoPtr const url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestInfoPtr expected_url_request = mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(kExpectedUrl);
  expected_url_request->headers = {"accept: application/json"};
  expected_url_request->content = kExpectedContent;
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethodType::kPost;

  EXPECT_EQ(url_request, expected_url_request);
}

TEST_F(BatAdsCreateOptedInConfirmationUrlRequestBuilderTest,
       BuildUrlForOtherCountry) {
  // Arrange
  GlobalState::GetInstance()->Flags().environment_type =
      mojom::EnvironmentType::kStaging;

  privacy::SetUnblindedTokens(/*count*/ 1);

  MockBuildChannel(BuildChannelType::kRelease);

  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"en_KY"};

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);
  CreateOptedInConfirmationUrlRequestBuilder url_request_builder(*confirmation);

  // Act
  mojom::UrlRequestInfoPtr const url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestInfoPtr expected_url_request = mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(kExpectedUrl);
  expected_url_request->headers = {"accept: application/json"};
  expected_url_request->content = kExpectedContent;
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethodType::kPost;

  EXPECT_EQ(url_request, expected_url_request);
}

TEST_F(BatAdsCreateOptedInConfirmationUrlRequestBuilderTest,
       BuildUrlForLargeAnonmityCountryAndNonReleaseBuildChannel) {
  // Arrange
  GlobalState::GetInstance()->Flags().environment_type =
      mojom::EnvironmentType::kStaging;

  privacy::SetUnblindedTokens(/*count*/ 1);

  MockBuildChannel(BuildChannelType::kNightly);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);
  CreateOptedInConfirmationUrlRequestBuilder url_request_builder(*confirmation);

  // Act
  mojom::UrlRequestInfoPtr const url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestInfoPtr expected_url_request = mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(kExpectedUrl);
  expected_url_request->headers = {"accept: application/json"};
  expected_url_request->content = kExpectedContent;
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethodType::kPost;

  EXPECT_EQ(url_request, expected_url_request);
}

TEST_F(BatAdsCreateOptedInConfirmationUrlRequestBuilderTest,
       BuildUrlForAnonymousCountryAndNonReleaseBuildChannel) {
  // Arrange
  GlobalState::GetInstance()->Flags().environment_type =
      mojom::EnvironmentType::kStaging;

  privacy::SetUnblindedTokens(/*count*/ 1);

  MockBuildChannel(BuildChannelType::kNightly);

  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"en_AS"};

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);
  CreateOptedInConfirmationUrlRequestBuilder url_request_builder(*confirmation);

  // Act
  mojom::UrlRequestInfoPtr const url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestInfoPtr expected_url_request = mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(kExpectedUrl);
  expected_url_request->headers = {"accept: application/json"};
  expected_url_request->content = kExpectedContent;
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethodType::kPost;

  EXPECT_EQ(url_request, expected_url_request);
}

TEST_F(BatAdsCreateOptedInConfirmationUrlRequestBuilderTest,
       BuildUrlForOtherCountryAndNonReleaseBuildChannel) {
  // Arrange
  GlobalState::GetInstance()->Flags().environment_type =
      mojom::EnvironmentType::kStaging;

  privacy::SetUnblindedTokens(/*count*/ 1);

  MockBuildChannel(BuildChannelType::kNightly);

  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"en_KY"};

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);
  CreateOptedInConfirmationUrlRequestBuilder url_request_builder(*confirmation);

  // Act
  mojom::UrlRequestInfoPtr const url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestInfoPtr expected_url_request = mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(kExpectedUrl);
  expected_url_request->headers = {"accept: application/json"};
  expected_url_request->content = kExpectedContent;
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethodType::kPost;

  EXPECT_EQ(url_request, expected_url_request);
}

}  // namespace brave_ads
