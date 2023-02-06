/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/redeem_unblinded_token/fetch_payment_token_url_request_builder.h"

#include "bat/ads/internal/account/confirmations/confirmation_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/flags/flag_manager.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsFetchPaymentTokenUrlRequestBuilderTest : public UnitTestBase {};

TEST_F(BatAdsFetchPaymentTokenUrlRequestBuilderTest, BuildUrl) {
  // Arrange
  FlagManager::GetInstance()->SetEnvironmentTypeForTesting(
      EnvironmentType::kStaging);

  privacy::SetUnblindedTokens(1);
  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  FetchPaymentTokenUrlRequestBuilder url_request_builder(*confirmation);

  // Act
  const mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestInfoPtr expected_url_request = mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(
      "https://anonymous.ads.bravesoftware.com/v3/confirmation/"
      "8b742869-6e4a-490c-ac31-31b49130098a/paymentToken");
  expected_url_request->method = mojom::UrlRequestMethodType::kGet;

  EXPECT_EQ(expected_url_request, url_request);
}

}  // namespace ads
