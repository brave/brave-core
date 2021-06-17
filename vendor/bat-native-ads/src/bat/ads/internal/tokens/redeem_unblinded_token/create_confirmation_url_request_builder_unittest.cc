/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/redeem_unblinded_token/create_confirmation_url_request_builder.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::Return;

namespace ads {

class BatAdsCreateConfirmationUrlRequestBuilderTest : public UnitTestBase {
 protected:
  BatAdsCreateConfirmationUrlRequestBuilderTest() = default;

  ~BatAdsCreateConfirmationUrlRequestBuilderTest() override = default;

  ConfirmationInfo GetConfirmationForType(const ConfirmationType type) {
    ConfirmationInfo confirmation;

    confirmation.id = "d990ed8d-d739-49fb-811b-c2e02158fb60";
    confirmation.creative_instance_id = "546fe7b0-5047-4f28-a11c-81f14edcf0f6";

    confirmation.type = type;

    const std::string blinded_token_base64 =
        "PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=";
    confirmation.blinded_payment_token =
        BlindedToken::decode_base64(blinded_token_base64);

    confirmation.credential =
        "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdW"
        "Rkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZl"
        "SW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNm"
        "MGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0"
        "dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJY"
        "MnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIs"
        "InQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUy"
        "WEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=";

    return confirmation;
  }
};

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForLargeAnonmityCountryForRPill) {
  // Arrange
  SysInfo sys_info;
  sys_info.is_uncertain_future = true;
  SetSysInfo(sys_info);

  const ConfirmationInfo confirmation =
      GetConfirmationForType(ConfirmationType::kViewed);

  SetBuildChannel(true, "release");

  MockLocaleHelper(locale_helper_mock_, "en-US");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  UrlRequestPtr expected_url_request = UrlRequest::New();
  expected_url_request->url =
      R"(https://ads-serve.brave.software/v1/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=)";
  expected_url_request->headers = {
      R"(Via: 1.0 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content =
      R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"type":"view"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = UrlRequestMethod::POST;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForAnonymousCountryForRPill) {
  // Arrange
  SysInfo sys_info;
  sys_info.is_uncertain_future = true;
  SetSysInfo(sys_info);

  const ConfirmationInfo confirmation =
      GetConfirmationForType(ConfirmationType::kViewed);

  SetBuildChannel(true, "release");

  MockLocaleHelper(locale_helper_mock_, "en-AS");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  UrlRequestPtr expected_url_request = UrlRequest::New();
  expected_url_request->url =
      R"(https://ads-serve.brave.software/v1/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=)";
  expected_url_request->headers = {
      R"(Via: 1.0 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content =
      R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"type":"view"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = UrlRequestMethod::POST;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForOtherCountryForRPill) {
  // Arrange
  SysInfo sys_info;
  sys_info.is_uncertain_future = true;
  SetSysInfo(sys_info);

  const ConfirmationInfo confirmation =
      GetConfirmationForType(ConfirmationType::kViewed);

  SetBuildChannel(true, "release");

  MockLocaleHelper(locale_helper_mock_, "en-KY");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  UrlRequestPtr expected_url_request = UrlRequest::New();
  expected_url_request->url =
      R"(https://ads-serve.brave.software/v1/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=)";
  expected_url_request->headers = {
      R"(Via: 1.0 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content =
      R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"type":"view"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = UrlRequestMethod::POST;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForLargeAnonmityCountryAndNonReleaseBuildChannelForRPill) {
  // Arrange
  SysInfo sys_info;
  sys_info.is_uncertain_future = true;
  SetSysInfo(sys_info);

  const ConfirmationInfo confirmation =
      GetConfirmationForType(ConfirmationType::kViewed);

  SetBuildChannel(false, "beta");

  MockLocaleHelper(locale_helper_mock_, "en-US");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  UrlRequestPtr expected_url_request = UrlRequest::New();
  expected_url_request->url =
      R"(https://ads-serve.brave.software/v1/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=)";
  expected_url_request->headers = {
      R"(Via: 1.0 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content =
      R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"type":"view"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = UrlRequestMethod::POST;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForAnonymousCountryAndNonReleaseBuildChannelForRPill) {
  // Arrange
  SysInfo sys_info;
  sys_info.is_uncertain_future = true;
  SetSysInfo(sys_info);

  const ConfirmationInfo confirmation =
      GetConfirmationForType(ConfirmationType::kViewed);

  SetBuildChannel(false, "beta");

  MockLocaleHelper(locale_helper_mock_, "en-AS");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  UrlRequestPtr expected_url_request = UrlRequest::New();
  expected_url_request->url =
      R"(https://ads-serve.brave.software/v1/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=)";
  expected_url_request->headers = {
      R"(Via: 1.0 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content =
      R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"type":"view"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = UrlRequestMethod::POST;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForOtherCountryAndNonReleaseBuildChannelForRPill) {
  // Arrange
  SysInfo sys_info;
  sys_info.is_uncertain_future = true;
  SetSysInfo(sys_info);

  const ConfirmationInfo confirmation =
      GetConfirmationForType(ConfirmationType::kViewed);

  SetBuildChannel(false, "beta");

  MockLocaleHelper(locale_helper_mock_, "en-KY");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  UrlRequestPtr expected_url_request = UrlRequest::New();
  expected_url_request->url =
      R"(https://ads-serve.brave.software/v1/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=)";
  expected_url_request->headers = {
      R"(Via: 1.1 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content =
      R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"type":"view"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = UrlRequestMethod::POST;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForLargeAnonmityCountryForBPill) {
  // Arrange
  SysInfo sys_info;
  sys_info.is_uncertain_future = false;
  SetSysInfo(sys_info);

  const ConfirmationInfo confirmation =
      GetConfirmationForType(ConfirmationType::kViewed);

  SetBuildChannel(true, "release");

  MockLocaleHelper(locale_helper_mock_, "en-US");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  UrlRequestPtr expected_url_request = UrlRequest::New();
  expected_url_request->url =
      R"(https://ads-serve.brave.software/v1/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=)";
  expected_url_request->headers = {
      R"(Via: 1.1 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content =
      R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"type":"view"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = UrlRequestMethod::POST;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForAnonymousCountryForBPill) {
  // Arrange
  SysInfo sys_info;
  sys_info.is_uncertain_future = false;
  SetSysInfo(sys_info);

  const ConfirmationInfo confirmation =
      GetConfirmationForType(ConfirmationType::kViewed);

  SetBuildChannel(true, "release");

  MockLocaleHelper(locale_helper_mock_, "en-AS");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  UrlRequestPtr expected_url_request = UrlRequest::New();
  expected_url_request->url =
      R"(https://ads-serve.brave.software/v1/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=)";
  expected_url_request->headers = {
      R"(Via: 1.1 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content =
      R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"type":"view"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = UrlRequestMethod::POST;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForOtherCountryForBPill) {
  // Arrange
  SysInfo sys_info;
  sys_info.is_uncertain_future = false;
  SetSysInfo(sys_info);

  const ConfirmationInfo confirmation =
      GetConfirmationForType(ConfirmationType::kViewed);

  SetBuildChannel(true, "release");

  MockLocaleHelper(locale_helper_mock_, "en-KY");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  UrlRequestPtr expected_url_request = UrlRequest::New();
  expected_url_request->url =
      R"(https://ads-serve.brave.software/v1/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=)";
  expected_url_request->headers = {
      R"(Via: 1.1 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content =
      R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"type":"view"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = UrlRequestMethod::POST;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForLargeAnonmityCountryAndNonReleaseBuildChannelForBPill) {
  // Arrange
  SysInfo sys_info;
  sys_info.is_uncertain_future = false;
  SetSysInfo(sys_info);

  const ConfirmationInfo confirmation =
      GetConfirmationForType(ConfirmationType::kViewed);

  SetBuildChannel(false, "beta");

  MockLocaleHelper(locale_helper_mock_, "en-US");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  UrlRequestPtr expected_url_request = UrlRequest::New();
  expected_url_request->url =
      R"(https://ads-serve.brave.software/v1/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=)";
  expected_url_request->headers = {
      R"(Via: 1.1 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content =
      R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"type":"view"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = UrlRequestMethod::POST;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForAnonymousCountryAndNonReleaseBuildChannelForBPill) {
  // Arrange
  SysInfo sys_info;
  sys_info.is_uncertain_future = false;
  SetSysInfo(sys_info);

  const ConfirmationInfo confirmation =
      GetConfirmationForType(ConfirmationType::kViewed);

  SetBuildChannel(false, "beta");

  MockLocaleHelper(locale_helper_mock_, "en-AS");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  UrlRequestPtr expected_url_request = UrlRequest::New();
  expected_url_request->url =
      R"(https://ads-serve.brave.software/v1/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=)";
  expected_url_request->headers = {
      R"(Via: 1.1 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content =
      R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"type":"view"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = UrlRequestMethod::POST;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
       BuildUrlForOtherCountryAndNonReleaseBuildChannelForBPill) {
  // Arrange
  SysInfo sys_info;
  sys_info.is_uncertain_future = false;
  SetSysInfo(sys_info);

  const ConfirmationInfo confirmation =
      GetConfirmationForType(ConfirmationType::kViewed);

  SetBuildChannel(false, "beta");

  MockLocaleHelper(locale_helper_mock_, "en-KY");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  UrlRequestPtr expected_url_request = UrlRequest::New();
  expected_url_request->url =
      R"(https://ads-serve.brave.software/v1/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=)";
  expected_url_request->headers = {
      R"(Via: 1.1 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
      R"(accept: application/json)"};
  expected_url_request->content =
      R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"type":"view"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = UrlRequestMethod::POST;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

}  // namespace ads
