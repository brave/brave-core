/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/redeem_unblinded_token/fetch_payment_token_url_request_builder.h"

#include "bat/ads/internal/account/confirmations/confirmations_unittest_util.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/flags/flag_manager_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsFetchPaymentTokenUrlRequestBuilderTest : public UnitTestBase {
 protected:
  BatAdsFetchPaymentTokenUrlRequestBuilderTest() = default;

  ~BatAdsFetchPaymentTokenUrlRequestBuilderTest() override = default;
};

TEST_F(BatAdsFetchPaymentTokenUrlRequestBuilderTest, BuildUrl) {
  // Arrange
  SetEnvironmentTypeForTesting(EnvironmentType::kStaging);

  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo confirmation = BuildConfirmation(
      /* id */ "d990ed8d-d739-49fb-811b-c2e02158fb60",
      /* transaction_id */ "8b742869-6e4a-490c-ac31-31b49130098a",
      /* creative_instance_id */ "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
      ConfirmationType::kViewed, AdType::kNotificationAd);

  FetchPaymentTokenUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestInfoPtr expected_url_request = mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(
      R"(https://anonymous.ads.bravesoftware.com/v2/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/paymentToken)");
  expected_url_request->method = mojom::UrlRequestMethodType::kGet;

  EXPECT_EQ(expected_url_request, url_request);
}

}  // namespace ads
