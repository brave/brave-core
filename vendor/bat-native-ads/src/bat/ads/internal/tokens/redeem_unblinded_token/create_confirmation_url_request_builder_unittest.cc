/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/redeem_unblinded_token/create_confirmation_url_request_builder.h"

#include "bat/ads/internal/account/confirmations/confirmations_unittest_util.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kExpectedUrl[] =
    R"(https://ads-serve.bravesoftware.com/v2/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlbnNcIjpbXCJFdjVKRTQvOVRaSS81VHF5TjlKV2ZKMVRvMEhCd1F3MnJXZUFQY2RqWDNRPVwiXSxcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJwdWJsaWNLZXlcIjpcIlJKMmkvby9wWmtySCtpMGFHRU1ZMUc5Rlh0ZDdRN2dmUmkzWWROUm5ERGs9XCIsXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiZzV5R1FhcGNHcVkxeUhjMXV6TUhyT1ZhM2dHRkliTjkwUmlkcnlmakF0dTlyQzMwRmk5K3RVWGFrYmVYYVZKZDZVVkdub2w4ZW5MQWJQd0ZuNGpzc0E9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=)";
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
  mojom::SysInfo sys_info;
  sys_info.is_uncertain_future = true;
  SetSysInfo(sys_info);

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kAdNotification);

  SetBuildChannel(true, "release");

  MockLocaleHelper(locale_helper_mock_, "en-US");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = kExpectedUrl;
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
  mojom::SysInfo sys_info;
  sys_info.is_uncertain_future = true;
  SetSysInfo(sys_info);

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kAdNotification);

  SetBuildChannel(true, "release");

  MockLocaleHelper(locale_helper_mock_, "en-AS");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = kExpectedUrl;
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
  mojom::SysInfo sys_info;
  sys_info.is_uncertain_future = true;
  SetSysInfo(sys_info);

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kAdNotification);

  SetBuildChannel(true, "release");

  MockLocaleHelper(locale_helper_mock_, "en-KY");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = kExpectedUrl;
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
  mojom::SysInfo sys_info;
  sys_info.is_uncertain_future = true;
  SetSysInfo(sys_info);

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kAdNotification);

  SetBuildChannel(false, "beta");

  MockLocaleHelper(locale_helper_mock_, "en-US");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = kExpectedUrl;
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
  mojom::SysInfo sys_info;
  sys_info.is_uncertain_future = true;
  SetSysInfo(sys_info);

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kAdNotification);

  SetBuildChannel(false, "beta");

  MockLocaleHelper(locale_helper_mock_, "en-AS");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = kExpectedUrl;
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
  mojom::SysInfo sys_info;
  sys_info.is_uncertain_future = true;
  SetSysInfo(sys_info);

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kAdNotification);

  SetBuildChannel(false, "beta");

  MockLocaleHelper(locale_helper_mock_, "en-KY");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = kExpectedUrl;
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
  mojom::SysInfo sys_info;
  sys_info.is_uncertain_future = false;
  SetSysInfo(sys_info);

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kAdNotification);

  SetBuildChannel(true, "release");

  MockLocaleHelper(locale_helper_mock_, "en-US");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = kExpectedUrl;
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
  mojom::SysInfo sys_info;
  sys_info.is_uncertain_future = false;
  SetSysInfo(sys_info);

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kAdNotification);

  SetBuildChannel(true, "release");

  MockLocaleHelper(locale_helper_mock_, "en-AS");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = kExpectedUrl;
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
  mojom::SysInfo sys_info;
  sys_info.is_uncertain_future = false;
  SetSysInfo(sys_info);

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kAdNotification);

  SetBuildChannel(true, "release");

  MockLocaleHelper(locale_helper_mock_, "en-KY");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = kExpectedUrl;
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
  mojom::SysInfo sys_info;
  sys_info.is_uncertain_future = false;
  SetSysInfo(sys_info);

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kAdNotification);

  SetBuildChannel(false, "beta");

  MockLocaleHelper(locale_helper_mock_, "en-US");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = kExpectedUrl;
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
  mojom::SysInfo sys_info;
  sys_info.is_uncertain_future = false;
  SetSysInfo(sys_info);

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kAdNotification);

  SetBuildChannel(false, "beta");

  MockLocaleHelper(locale_helper_mock_, "en-AS");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = kExpectedUrl;
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
  mojom::SysInfo sys_info;
  sys_info.is_uncertain_future = false;
  SetSysInfo(sys_info);

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kAdNotification);

  SetBuildChannel(false, "beta");

  MockLocaleHelper(locale_helper_mock_, "en-KY");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url = kExpectedUrl;
  expected_url_request->headers = {
      R"(Via: 1.0 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content = kExpectedContent;
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethod::kPost;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

}  // namespace ads
