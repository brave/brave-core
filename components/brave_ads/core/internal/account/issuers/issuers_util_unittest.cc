/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsIssuersUtilTest : public UnitTestBase {};

TEST_F(BraveAdsIssuersUtilTest, HasIssuersChanged) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act
  const IssuersInfo issuers = test::BuildIssuers(
      3'600'000,
      {{"Nj2NZ6nJUsK5MJ9ga9tfyctxzpT+GlvENF2TRHU4kBg=", 0.0},
       {"TFQCiRJocOh0A8+qHQvdu3V/lDpGsZHJOnZzqny6rFg=", 0.0}},
      {{"PmXS59VTEVIPZckOqGdpjisDidUbhLGbhAhN5tmfhhs=", 0.1},
       {"Bgk5gT+b96iSr3nD5nuTM/yGQ5klrIe6VC6DDdM6sFs=", 0.0}});

  // Assert
  EXPECT_TRUE(HasIssuersChanged(issuers));
}

TEST_F(BraveAdsIssuersUtilTest, HasIssuersChangedOnInitialFetch) {
  // Act
  const IssuersInfo issuers = test::BuildIssuers(
      3'600'000,
      {{"Nj2NZ6nJUsK5MJ9ga9tfyctxzpT+GlvENF2TRHU4kBg=", 0.0},
       {"TFQCiRJocOh0A8+qHQvdu3V/lDpGsZHJOnZzqny6rFg=", 0.0}},
      {{"PmXS59VTEVIPZckOqGdpjisDidUbhLGbhAhN5tmfhhs=", 0.1},
       {"Bgk5gT+b96iSr3nD5nuTM/yGQ5klrIe6VC6DDdM6sFs=", 0.0}});

  // Assert
  EXPECT_TRUE(HasIssuersChanged(issuers));
}

TEST_F(BraveAdsIssuersUtilTest, HasIssuersNotChanged) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act
  const IssuersInfo issuers = test::BuildIssuers(
      7'200'000,
      {{"bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=", 0.0},
       {"QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=", 0.0}},
      {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
       {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}});

  // Assert
  EXPECT_FALSE(HasIssuersChanged(issuers));
}

TEST_F(BraveAdsIssuersUtilTest, IssuerDoesExistForConfirmationsType) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act & Assert
  EXPECT_TRUE(IssuerExistsForType(IssuerType::kConfirmations));
}

TEST_F(BraveAdsIssuersUtilTest, IssuerDoesNotExistForConfirmationsType) {
  // Arrange
  const IssuersInfo issuers = test::BuildIssuers(
      7'200'000, {},
      {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
       {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}});

  SetIssuers(issuers);

  // Act & Assert
  EXPECT_FALSE(IssuerExistsForType(IssuerType::kConfirmations));
}

TEST_F(BraveAdsIssuersUtilTest, IssuerDoesExistForPaymentsType) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act & Assert
  EXPECT_TRUE(IssuerExistsForType(IssuerType::kPayments));
}

TEST_F(BraveAdsIssuersUtilTest, IssuerDoesNotExistForPaymentsType) {
  // Arrange
  const IssuersInfo issuers = test::BuildIssuers(
      7'200'000,
      {{"bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=", 0.0},
       {"cKo0rk1iS8Obgyni0X3RRoydDIGHsivTkfX/TM1Xl24=", 0.0}},
      {});

  SetIssuers(issuers);

  // Act & Assert
  EXPECT_FALSE(IssuerExistsForType(IssuerType::kPayments));
}

TEST_F(BraveAdsIssuersUtilTest, PublicKeyDoesExistForConfirmationsType) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act & Assert
  EXPECT_TRUE(PublicKeyExistsForIssuerType(
      IssuerType::kConfirmations,
      /*public_key=*/"QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g="));
}

TEST_F(BraveAdsIssuersUtilTest, PublicKeyDoesNotExistForConfirmationsType) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act & Assert
  EXPECT_FALSE(PublicKeyExistsForIssuerType(
      IssuerType::kConfirmations,
      /*public_key=*/"Nj2NZ6nJUsK5MJ9ga9tfyctxzpT+GlvENF2TRHU4kBg="));
}

TEST_F(BraveAdsIssuersUtilTest, PublicKeyDoesExistForPaymentsType) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act & Assert
  EXPECT_TRUE(PublicKeyExistsForIssuerType(
      IssuerType::kPayments,
      /*public_key=*/"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU="));
}

TEST_F(BraveAdsIssuersUtilTest, PublicKeyDoesNotExistForPaymentsType) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act & Assert
  EXPECT_FALSE(PublicKeyExistsForIssuerType(
      IssuerType::kPayments,
      /*public_key=*/"zNWjpwIbghgXvTol3XPLKV3NJoEFtvUoPMiKstiWm3A="));
}

TEST_F(BraveAdsIssuersUtilTest, GetIssuersForType) {
  // Arrange
  const IssuersInfo issuers = test::BuildIssuers(
      7'200'000,
      {{"bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=", 0.0},
       {"QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=", 0.0}},
      {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
       {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}});

  // Act & Assert
  IssuerInfo expected_issuer;
  expected_issuer.type = IssuerType::kPayments;
  expected_issuer.public_keys = {
      {"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
      {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}};
  EXPECT_EQ(expected_issuer, GetIssuerForType(issuers, IssuerType::kPayments));
}

TEST_F(BraveAdsIssuersUtilTest, DoNotGetIssuersForMissingType) {
  // Arrange
  const IssuersInfo issuers = test::BuildIssuers(
      7'200'000,
      {{"bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=", 0.0},
       {"QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=", 0.0}},
      {});

  // Act & Assert
  EXPECT_FALSE(GetIssuerForType(issuers, IssuerType::kPayments));
}

TEST_F(BraveAdsIssuersUtilTest, IsIssuersValid) {
  // Arrange
  const IssuersInfo issuers = test::BuildIssuers(
      7'200'000,
      {{"bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=", 0.0},
       {"QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=", 0.0}},
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
      7'200'000,
      {{"bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=", 0.0},
       {"QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=", 0.0}},
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

}  // namespace brave_ads
