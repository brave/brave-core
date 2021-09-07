/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/redeem_unblinded_token/fetch_payment_token_url_request_builder.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsFetchPaymentTokenUrlRequestBuilderTest, BuildUrl) {
  // Arrange
  ConfirmationInfo confirmation;
  confirmation.id = "546fe7b0-5047-4f28-a11c-81f14edcf0f6";
  confirmation.creative_instance_id = "6b233edf-4c0a-4029-a0a7-6a5d96fb769e";
  confirmation.type = ConfirmationType::kViewed;

  FetchPaymentTokenUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url =
      R"(https://ads-serve.brave.software/v1/confirmation/546fe7b0-5047-4f28-a11c-81f14edcf0f6/paymentToken)";
  expected_url_request->method = mojom::UrlRequestMethod::kGet;

  EXPECT_EQ(expected_url_request, url_request);
}

}  // namespace ads
