/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsIssuersUtilTest : public test::TestBase {};

TEST_F(BraveAdsIssuersUtilTest, IsIssuersValid) {
  // Arrange
  const IssuersInfo issuers = test::BuildIssuers(
      /*ping=*/7'200'000,
      /*confirmation_token_issuer_public_keys=*/
      {{"bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=", 0.0},
       {"QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=", 0.0}},
      /*payment_token_issuer_public_keys=*/
      {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
       {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1},
       {"XovQyvVWM8ez0mAzTtfqgPIbSpH5/idv8w0KJxhirwA=", 0.1},
       {"wAcnJtb34Asykf+2jrTWrjFiaTqilklZ6bxLyR3LyFo=", 0.1},
       {"ZvzeYOT1geUQXfOsYXBxZj/H26IfiBUVodHl51j68xI=", 0.1},
       {"JlOezORiqLkFkvapoNRGWcMH3/g09/7M2UPEwMjRpFE=", 0.1},
       {"hJP1nDjTdHcVDw347oH0XO+XBPPh5wZA2xWZE8QUSSA=", 0.1}});

  // Act & Assert
  EXPECT_TRUE(IsIssuersValid(issuers));
}

TEST_F(BraveAdsIssuersUtilTest, IsIssuersInvalid) {
  // Arrange
  const IssuersInfo issuers = test::BuildIssuers(
      /*ping=*/7'200'000,
      /*confirmation_token_issuer_public_keys=*/
      {{"bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=", 0.0},
       {"QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=", 0.0}},
      /*payment_token_issuer_public_keys=*/
      {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
       {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1},
       {"XovQyvVWM8ez0mAzTtfqgPIbSpH5/idv8w0KJxhirwA=", 0.1},
       {"wAcnJtb34Asykf+2jrTWrjFiaTqilklZ6bxLyR3LyFo=", 0.1},
       {"ZvzeYOT1geUQXfOsYXBxZj/H26IfiBUVodHl51j68xI=", 0.1},
       {"JlOezORiqLkFkvapoNRGWcMH3/g09/7M2UPEwMjRpFE=", 0.1},
       {"hJP1nDjTdHcVDw347oH0XO+XBPPh5wZA2xWZE8QUSSA=", 0.1},
       {"+iyhYDv7W6cuFAD1tzsJIEQKEStTX9B/Tt62tqt+tG0=", 0.1},
       {"oMx8hW6w8w/AUGoJMAMbrdeV1FoqrRkoR3BVa+5lDDk=", 0.1}});

  // Act & Assert
  EXPECT_FALSE(IsIssuersValid(issuers));
}

TEST_F(BraveAdsIssuersUtilTest, HasIssuers) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act & Assert
  EXPECT_TRUE(HasIssuers());
}

TEST_F(BraveAdsIssuersUtilTest, DoesNotHaveIssuers) {
  // Act & Assert
  EXPECT_FALSE(HasIssuers());
}

TEST_F(BraveAdsIssuersUtilTest, HasIssuersChanged) {
  // Arrange
  test::BuildAndSetIssuers();

  const IssuersInfo issuers = test::BuildIssuers(
      /*ping=*/3'600'000,
      /*confirmation_token_issuer_public_keys=*/
      {{"Nj2NZ6nJUsK5MJ9ga9tfyctxzpT+GlvENF2TRHU4kBg=", 0.0},
       {"TFQCiRJocOh0A8+qHQvdu3V/lDpGsZHJOnZzqny6rFg=", 0.0}},
      /*payment_token_issuer_public_keys=*/
      {{"PmXS59VTEVIPZckOqGdpjisDidUbhLGbhAhN5tmfhhs=", 0.1},
       {"Bgk5gT+b96iSr3nD5nuTM/yGQ5klrIe6VC6DDdM6sFs=", 0.0}});

  // Act & Assert
  EXPECT_TRUE(HasIssuersChanged(issuers));
}

TEST_F(BraveAdsIssuersUtilTest, HasIssuersChangedOnInitialFetch) {
  // Arrange
  const IssuersInfo issuers = test::BuildIssuers(
      /*ping=*/3'600'000,
      /*confirmation_token_issuer_public_keys=*/
      {{"Nj2NZ6nJUsK5MJ9ga9tfyctxzpT+GlvENF2TRHU4kBg=", 0.0},
       {"TFQCiRJocOh0A8+qHQvdu3V/lDpGsZHJOnZzqny6rFg=", 0.0}},
      /*payment_token_issuer_public_keys=*/
      {{"PmXS59VTEVIPZckOqGdpjisDidUbhLGbhAhN5tmfhhs=", 0.1},
       {"Bgk5gT+b96iSr3nD5nuTM/yGQ5klrIe6VC6DDdM6sFs=", 0.0}});

  // Act & Assert
  EXPECT_TRUE(HasIssuersChanged(issuers));
}

TEST_F(BraveAdsIssuersUtilTest, HasIssuersNotChanged) {
  // Arrange
  test::BuildAndSetIssuers();

  const IssuersInfo issuers = test::BuildIssuers(
      /*ping=*/7'200'000,
      /*confirmation_token_issuer_public_keys=*/
      {{"bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=", 0.0},
       {"QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=", 0.0}},
      /*payment_token_issuer_public_keys=*/
      {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
       {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}});

  // Act & Assert
  EXPECT_FALSE(HasIssuersChanged(issuers));
}

}  // namespace brave_ads
