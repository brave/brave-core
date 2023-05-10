/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/redeem_confirmation/url_request_builders/create_opted_in_confirmation_url_request_builder_util.h"

#include "brave/components/brave_ads/core/internal/account/confirmations/redeem_confirmation/url_request_builders/create_opted_in_confirmation_url_request_builder_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreateOptedInConfirmationUrlRequestBuilderUtilTest, GetPath) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(
      R"(/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlbnNcIjpbXCJOSktBRFZNTlphZkEyVUFDTlkyVGxSZWFDSG43cW5mWndLZEdrd3dtSzBFPVwiXSxcImJ1aWxkQ2hhbm5lbFwiOlwicmVsZWFzZVwiLFwiY291bnRyeUNvZGVcIjpcIlVTXCIsXCJjcmVhdGl2ZUluc3RhbmNlSWRcIjpcIjQ4OWIxZGNjLWQ5MzgtNDFjZi05MmE4LWJhY2VhMGFiYjQ5MFwiLFwicGF5bG9hZFwiOnt9LFwicGxhdGZvcm1cIjpcIndpbmRvd3NcIixcInB1YmxpY0tleVwiOlwiTEFxSnhmeVhnV241MlhRQXFxRVhPMFAvVjQ2dVpjU0kvb1hMZFp3ekFnRT1cIixcInN0dWRpZXNcIjpbe1wiZ3JvdXBcIjpcIkV4Y2x1ZGVBZElmV2l0aGluVGltZVdpbmRvdz0waFwiLFwibmFtZVwiOlwiQnJhdmVBZHMuRnJlcXVlbmN5Q2FwcGluZ1N0dWR5XCJ9LHtcImdyb3VwXCI6XCJUcmlnZ2Vycz1FTVBUWS9UaHJlc2hvbGQ9MC4wL0lkbGVUaW1lVGhyZXNob2xkPTVcIixcIm5hbWVcIjpcIkJyYXZlQWRzLlVzZXJBY3Rpdml0eVN0dWR5XCJ9XSxcInR5cGVcIjpcImRpc21pc3NcIn0iLCJzaWduYXR1cmUiOiJlU3RqbkMzamVUWVBsSXJmb2JSdnBiL05oVCtZQmc1YlllVHlLN3NUcHd3em1pc0FOY0FUU2NSMXRYQkwzOTdFY1NXdVJ5ZHhwQk9ReFF5a0VHQUwrQT09IiwidCI6IlN4UllQVTJVamFNVjhhanV4QmVQRGlCWStsQXM3VHdIc2pubzVHYUJhc2hlZ1BZMmkyUGtTbSswWUIzT0toQ2hlTmVSNWNrYmMvRmREbnFqVktxMW1BPT0ifQ==)",
      BuildCreateOptedInConfirmationUrlPath(
          kTransactionId, kCreateOptedInConfirmationCredential));
}

}  // namespace brave_ads
