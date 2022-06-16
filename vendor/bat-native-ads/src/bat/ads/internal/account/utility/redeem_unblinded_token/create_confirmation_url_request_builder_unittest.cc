/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/redeem_unblinded_token/create_confirmation_url_request_builder.h"

#include "bat/ads/ads.h"
#include "bat/ads/internal/account/confirmations/confirmations_unittest_util.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_mock_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kExpectedUrl[] =
    R"(https://anonymous.ads.bravesoftware.com/v2/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlbnNcIjpbXCJFdjVKRTQvOVRaSS81VHF5TjlKV2ZKMVRvMEhCd1F3MnJXZUFQY2RqWDNRPVwiXSxcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJwdWJsaWNLZXlcIjpcIlJKMmkvby9wWmtySCtpMGFHRU1ZMUc5Rlh0ZDdRN2dmUmkzWWROUm5ERGs9XCIsXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiZzV5R1FhcGNHcVkxeUhjMXV6TUhyT1ZhM2dHRkliTjkwUmlkcnlmakF0dTlyQzMwRmk5K3RVWGFrYmVYYVZKZDZVVkdub2w4ZW5MQWJQd0ZuNGpzc0E9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=)";
constexpr char kExpectedContent[] =
    R"({"blindedPaymentTokens":["Ev5JE4/9TZI/5TqyN9JWfJ1To0HBwQw2rWeAPcdjX3Q="],"creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=","type":"view"})";
}  // namespace

class BatAdsCreateConfirmationUrlRequestBuilderTest : public UnitTestBase {
 protected:
  BatAdsCreateConfirmationUrlRequestBuilderTest() = default;

  ~BatAdsCreateConfirmationUrlRequestBuilderTest() override = default;
};

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForLargeAnonmityCountryForRPill) {
  // Arrange
  SysInfo().is_uncertain_future = true;

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "8b742869-6e4a-490c-ac31-31b49130098a",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kNotificationAd);

  MockBuildChannel(BuildChannelType::kRelease);

  MockLocaleHelper(locale_helper_mock_, "en-US");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = GURL(kExpectedUrl);
  expected_url_request->headers = {
      R"(Via: 1.1 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content = kExpectedContent;
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethod::kPost;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForAnonymousCountryForRPill) {
  // Arrange
  SysInfo().is_uncertain_future = true;

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "8b742869-6e4a-490c-ac31-31b49130098a",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kNotificationAd);

  MockBuildChannel(BuildChannelType::kRelease);

  MockLocaleHelper(locale_helper_mock_, "en-AS");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = GURL(kExpectedUrl);
  expected_url_request->headers = {
      R"(Via: 1.1 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content = kExpectedContent;
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethod::kPost;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForOtherCountryForRPill) {
  // Arrange
  SysInfo().is_uncertain_future = true;

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "8b742869-6e4a-490c-ac31-31b49130098a",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kNotificationAd);

  MockBuildChannel(BuildChannelType::kRelease);

  MockLocaleHelper(locale_helper_mock_, "en-KY");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = GURL(kExpectedUrl);
  expected_url_request->headers = {
      R"(Via: 1.1 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content = kExpectedContent;
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethod::kPost;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForLargeAnonmityCountryAndNonReleaseBuildChannelForRPill) {
  // Arrange
  SysInfo().is_uncertain_future = true;

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "8b742869-6e4a-490c-ac31-31b49130098a",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kNotificationAd);

  MockBuildChannel(BuildChannelType::kNightly);

  MockLocaleHelper(locale_helper_mock_, "en-US");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = GURL(kExpectedUrl);
  expected_url_request->headers = {
      R"(Via: 1.1 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content = kExpectedContent;
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethod::kPost;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForAnonymousCountryAndNonReleaseBuildChannelForRPill) {
  // Arrange
  SysInfo().is_uncertain_future = true;

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "8b742869-6e4a-490c-ac31-31b49130098a",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kNotificationAd);

  MockBuildChannel(BuildChannelType::kNightly);

  MockLocaleHelper(locale_helper_mock_, "en-AS");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = GURL(kExpectedUrl);
  expected_url_request->headers = {
      R"(Via: 1.1 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content = kExpectedContent;
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethod::kPost;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForOtherCountryAndNonReleaseBuildChannelForRPill) {
  // Arrange
  SysInfo().is_uncertain_future = true;

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "8b742869-6e4a-490c-ac31-31b49130098a",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kNotificationAd);

  MockBuildChannel(BuildChannelType::kNightly);

  MockLocaleHelper(locale_helper_mock_, "en-KY");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = GURL(kExpectedUrl);
  expected_url_request->headers = {
      R"(Via: 1.1 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content = kExpectedContent;
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethod::kPost;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForLargeAnonmityCountryForBPill) {
  // Arrange
  SysInfo().is_uncertain_future = false;

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "8b742869-6e4a-490c-ac31-31b49130098a",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kNotificationAd);

  MockBuildChannel(BuildChannelType::kRelease);

  MockLocaleHelper(locale_helper_mock_, "en-US");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = GURL(kExpectedUrl);
  expected_url_request->headers = {
      R"(Via: 1.0 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content = kExpectedContent;
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethod::kPost;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForAnonymousCountryForBPill) {
  // Arrange
  SysInfo().is_uncertain_future = false;

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "8b742869-6e4a-490c-ac31-31b49130098a",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kNotificationAd);

  MockBuildChannel(BuildChannelType::kRelease);

  MockLocaleHelper(locale_helper_mock_, "en-AS");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = GURL(kExpectedUrl);
  expected_url_request->headers = {
      R"(Via: 1.0 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content = kExpectedContent;
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethod::kPost;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForOtherCountryForBPill) {
  // Arrange
  SysInfo().is_uncertain_future = false;

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "8b742869-6e4a-490c-ac31-31b49130098a",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kNotificationAd);

  MockBuildChannel(BuildChannelType::kRelease);

  MockLocaleHelper(locale_helper_mock_, "en-KY");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = GURL(kExpectedUrl);
  expected_url_request->headers = {
      R"(Via: 1.0 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content = kExpectedContent;
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethod::kPost;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForLargeAnonmityCountryAndNonReleaseBuildChannelForBPill) {
  // Arrange
  SysInfo().is_uncertain_future = false;

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "8b742869-6e4a-490c-ac31-31b49130098a",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kNotificationAd);

  MockBuildChannel(BuildChannelType::kNightly);

  MockLocaleHelper(locale_helper_mock_, "en-US");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = GURL(kExpectedUrl);
  expected_url_request->headers = {
      R"(Via: 1.0 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content = kExpectedContent;
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethod::kPost;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForAnonymousCountryAndNonReleaseBuildChannelForBPill) {
  // Arrange
  SysInfo().is_uncertain_future = false;

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "8b742869-6e4a-490c-ac31-31b49130098a",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kNotificationAd);

  MockBuildChannel(BuildChannelType::kNightly);

  MockLocaleHelper(locale_helper_mock_, "en-AS");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = GURL(kExpectedUrl);
  expected_url_request->headers = {
      R"(Via: 1.0 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content = kExpectedContent;
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethod::kPost;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForOtherCountryAndNonReleaseBuildChannelForBPill) {
  // Arrange
  SysInfo().is_uncertain_future = false;

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "8b742869-6e4a-490c-ac31-31b49130098a",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kNotificationAd);

  MockBuildChannel(BuildChannelType::kNightly);

  MockLocaleHelper(locale_helper_mock_, "en-KY");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = GURL(kExpectedUrl);
  expected_url_request->headers = {
      R"(Via: 1.0 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content = kExpectedContent;
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethod::kPost;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

}  // namespace ads
