/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/redeem_unblinded_token/create_confirmation_url_request_builder.h"

#include <memory>
#include <string>
#include <vector>

#include "base/strings/stringprintf.h"
#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ads/internal/platform/platform_helper_mock.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::NiceMock;
using ::testing::Return;

namespace ads {

class BatAdsCreateConfirmationUrlRequestBuilderTest : public ::testing::Test {
 protected:
  BatAdsCreateConfirmationUrlRequestBuilderTest()
      : locale_helper_mock_(std::make_unique<
            NiceMock<brave_l10n::LocaleHelperMock>>()),
        platform_helper_mock_(std::make_unique<
            NiceMock<PlatformHelperMock>>()) {
    // You can do set-up work for each test here

    brave_l10n::LocaleHelper::GetInstance()->set_for_testing(
        locale_helper_mock_.get());

    PlatformHelper::GetInstance()->set_for_testing(platform_helper_mock_.get());
  }

  ~BatAdsCreateConfirmationUrlRequestBuilderTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    ON_CALL(*locale_helper_mock_, GetLocale())
        .WillByDefault(Return("en-US"));

    ON_CALL(*platform_helper_mock_, GetPlatformName())
        .WillByDefault(Return("test"));
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case

  ConfirmationInfo GetConfirmationForType(
       const ConfirmationType type) {
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

  std::unique_ptr<brave_l10n::LocaleHelperMock> locale_helper_mock_;
  std::unique_ptr<PlatformHelperMock> platform_helper_mock_;
};

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
    BuildUrlForLargeAnonmityCountry) {
  // Arrange
  const ConfirmationInfo confirmation =
      GetConfirmationForType(ConfirmationType::kViewed);

  SetBuildChannel(true, "release");

  ON_CALL(*locale_helper_mock_, GetLocale())
      .WillByDefault(Return("en-US"));

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  UrlRequestPtr expected_url_request = UrlRequest::New();
  expected_url_request->url = R"(https://ads-serve.brave.software/v1/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=)";
  expected_url_request->headers = {
    "accept: application/json"
  };
  expected_url_request->content = R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","buildChannel":"release","countryCode":"US","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"platform":"test","type":"view"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = UrlRequestMethod::POST;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
    BuildUrlForAnonymousCountry) {
  // Arrange
  const ConfirmationInfo confirmation =
      GetConfirmationForType(ConfirmationType::kViewed);

  SetBuildChannel(true, "release");

  ON_CALL(*locale_helper_mock_, GetLocale())
      .WillByDefault(Return("en-AS"));

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  UrlRequestPtr expected_url_request = UrlRequest::New();
  expected_url_request->url = R"(https://ads-serve.brave.software/v1/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=)";
  expected_url_request->headers = {
    "accept: application/json"
  };
  expected_url_request->content = R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","buildChannel":"release","countryCode":"??","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"platform":"test","type":"view"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = UrlRequestMethod::POST;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
    BuildUrlForOtherCountry) {
  // Arrange
  const ConfirmationInfo confirmation =
      GetConfirmationForType(ConfirmationType::kViewed);

  SetBuildChannel(true, "release");

  ON_CALL(*locale_helper_mock_, GetLocale())
      .WillByDefault(Return("en-KY"));

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  UrlRequestPtr expected_url_request = UrlRequest::New();
  expected_url_request->url = R"(https://ads-serve.brave.software/v1/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=)";
  expected_url_request->headers = {
    "accept: application/json"
  };
  expected_url_request->content = R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","buildChannel":"release","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"platform":"test","type":"view"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = UrlRequestMethod::POST;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
    BuildUrlForLargeAnonmityCountryAndNonReleaseBuildChannel) {
  // Arrange
  const ConfirmationInfo confirmation =
      GetConfirmationForType(ConfirmationType::kViewed);

  SetBuildChannel(false, "beta");

  ON_CALL(*locale_helper_mock_, GetLocale())
      .WillByDefault(Return("en-US"));

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  UrlRequestPtr expected_url_request = UrlRequest::New();
  expected_url_request->url = R"(https://ads-serve.brave.software/v1/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=)";
  expected_url_request->headers = {
    "accept: application/json"
  };
  expected_url_request->content = R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","buildChannel":"beta","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"platform":"test","type":"view"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = UrlRequestMethod::POST;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
    BuildUrlForAnonymousCountryAndNonReleaseBuildChannel) {
  // Arrange
  const ConfirmationInfo confirmation =
      GetConfirmationForType(ConfirmationType::kViewed);

  SetBuildChannel(false, "beta");

  ON_CALL(*locale_helper_mock_, GetLocale())
      .WillByDefault(Return("en-AS"));

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  UrlRequestPtr expected_url_request = UrlRequest::New();
  expected_url_request->url = R"(https://ads-serve.brave.software/v1/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=)";
  expected_url_request->headers = {
    "accept: application/json"
  };
  expected_url_request->content = R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","buildChannel":"beta","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"platform":"test","type":"view"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = UrlRequestMethod::POST;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

TEST_F(BatAdsCreateConfirmationUrlRequestBuilderTest,
    BuildUrlForOtherCountryAndNonReleaseBuildChannel) {
  // Arrange
  const ConfirmationInfo confirmation =
      GetConfirmationForType(ConfirmationType::kViewed);

  SetBuildChannel(false, "beta");

  ON_CALL(*locale_helper_mock_, GetLocale())
      .WillByDefault(Return("en-KY"));

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);

  // Act
  UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  UrlRequestPtr expected_url_request = UrlRequest::New();
  expected_url_request->url = R"(https://ads-serve.brave.software/v1/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=)";
  expected_url_request->headers = {
    "accept: application/json"
  };
  expected_url_request->content = R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","buildChannel":"beta","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"platform":"test","type":"view"})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = UrlRequestMethod::POST;

  EXPECT_TRUE(url_request.Equals(expected_url_request));
}

}  // namespace ads
