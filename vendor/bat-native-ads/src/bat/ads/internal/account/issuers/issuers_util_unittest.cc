/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/issuers/issuers_util.h"

#include "bat/ads/internal/account/issuers/issuers_info.h"
#include "bat/ads/internal/account/issuers/issuers_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/common/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsIssuersUtilTest : public UnitTestBase {};

TEST_F(BatAdsIssuersUtilTest, HasIssuersChanged) {
  // Arrange
  BuildAndSetIssuers();

  // Act
  const IssuersInfo issuers =
      BuildIssuers(3'600'000,
                   {{"Nj2NZ6nJUsK5MJ9ga9tfyctxzpT+GlvENF2TRHU4kBg=", 0.0},
                    {"TFQCiRJocOh0A8+qHQvdu3V/lDpGsZHJOnZzqny6rFg=", 0.0}},
                   {{"PmXS59VTEVIPZckOqGdpjisDidUbhLGbhAhN5tmfhhs=", 0.1},
                    {"Bgk5gT+b96iSr3nD5nuTM/yGQ5klrIe6VC6DDdM6sFs=", 0.0}});

  const bool has_changed = HasIssuersChanged(issuers);

  // Assert
  EXPECT_TRUE(has_changed);
}

TEST_F(BatAdsIssuersUtilTest, HasIssuersChangedOnInitialFetch) {
  // Arrange
  AdsClientHelper::GetInstance()->ClearPref(prefs::kIssuers);

  // Act
  const IssuersInfo issuers =
      BuildIssuers(3'600'000,
                   {{"Nj2NZ6nJUsK5MJ9ga9tfyctxzpT+GlvENF2TRHU4kBg=", 0.0},
                    {"TFQCiRJocOh0A8+qHQvdu3V/lDpGsZHJOnZzqny6rFg=", 0.0}},
                   {{"PmXS59VTEVIPZckOqGdpjisDidUbhLGbhAhN5tmfhhs=", 0.1},
                    {"Bgk5gT+b96iSr3nD5nuTM/yGQ5klrIe6VC6DDdM6sFs=", 0.0}});

  const bool has_changed = HasIssuersChanged(issuers);

  // Assert
  EXPECT_TRUE(has_changed);
}

TEST_F(BatAdsIssuersUtilTest, HasIssuersNotChanged) {
  // Arrange
  BuildAndSetIssuers();

  // Act
  const IssuersInfo issuers =
      BuildIssuers(7'200'000,
                   {{"JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=", 0.0},
                    {"crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=", 0.0}},
                   {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
                    {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}});

  const bool has_changed = HasIssuersChanged(issuers);

  // Assert
  EXPECT_FALSE(has_changed);
}

TEST_F(BatAdsIssuersUtilTest, IssuerDoesExistForConfirmationsType) {
  // Arrange
  BuildAndSetIssuers();

  // Act
  const bool does_exist = IssuerExistsForType(IssuerType::kConfirmations);

  // Assert
  EXPECT_TRUE(does_exist);
}

TEST_F(BatAdsIssuersUtilTest, IssuerDoesNotExistForConfirmationsType) {
  // Arrange
  const IssuersInfo issuers =
      BuildIssuers(/*ping*/ 7'200'000, /*confirmations_public_keys*/ {},
                   /*payments_public_keys*/
                   {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
                    {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}});

  SetIssuers(issuers);

  // Act
  const bool does_exist = IssuerExistsForType(IssuerType::kConfirmations);

  // Assert
  EXPECT_FALSE(does_exist);
}

TEST_F(BatAdsIssuersUtilTest, IssuerDoesExistForPaymentsType) {
  // Arrange
  BuildAndSetIssuers();

  // Act
  const bool does_exist = IssuerExistsForType(IssuerType::kPayments);

  // Assert
  EXPECT_TRUE(does_exist);
}

TEST_F(BatAdsIssuersUtilTest, IssuerDoesNotExistForPaymentsType) {
  // Arrange
  const IssuersInfo issuers =
      BuildIssuers(7'200'000,
                   {{"JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=", 0.0},
                    {"cKo0rk1iS8Obgyni0X3RRoydDIGHsivTkfX/TM1Xl24=", 0.0}},
                   {});

  SetIssuers(issuers);

  // Act
  const bool does_exist = IssuerExistsForType(IssuerType::kPayments);

  // Assert
  EXPECT_FALSE(does_exist);
}

TEST_F(BatAdsIssuersUtilTest, PublicKeyDoesExistForConfirmationsType) {
  // Arrange
  BuildAndSetIssuers();

  // Act
  const bool does_exist = PublicKeyExistsForIssuerType(
      IssuerType::kConfirmations,
      "crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=");

  // Assert
  EXPECT_TRUE(does_exist);
}

TEST_F(BatAdsIssuersUtilTest, PublicKeyDoesNotExistForConfirmationsType) {
  // Arrange
  BuildAndSetIssuers();

  // Act
  const bool does_exist = PublicKeyExistsForIssuerType(
      IssuerType::kConfirmations,
      "Nj2NZ6nJUsK5MJ9ga9tfyctxzpT+GlvENF2TRHU4kBg=");

  // Assert
  EXPECT_FALSE(does_exist);
}

TEST_F(BatAdsIssuersUtilTest, PublicKeyDoesExistForPaymentsType) {
  // Arrange
  BuildAndSetIssuers();

  // Act
  const bool does_exist = PublicKeyExistsForIssuerType(
      IssuerType::kPayments, "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=");

  // Assert
  EXPECT_TRUE(does_exist);
}

TEST_F(BatAdsIssuersUtilTest, PublicKeyDoesNotExistForPaymentsType) {
  // Arrange
  BuildAndSetIssuers();

  // Act
  const bool does_exist = PublicKeyExistsForIssuerType(
      IssuerType::kPayments, "zNWjpwIbghgXvTol3XPLKV3NJoEFtvUoPMiKstiWm3A=");

  // Assert
  EXPECT_FALSE(does_exist);
}

TEST_F(BatAdsIssuersUtilTest, GetIssuersForType) {
  // Arrange
  const IssuersInfo issuers =
      BuildIssuers(7'200'000,
                   {{"JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=", 0.0},
                    {"crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=", 0.0}},
                   {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
                    {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}});

  // Act
  const absl::optional<IssuerInfo> issuer =
      GetIssuerForType(issuers, IssuerType::kPayments);
  ASSERT_TRUE(issuer);

  // Assert
  IssuerInfo expected_issuer;
  expected_issuer.type = IssuerType::kPayments;
  expected_issuer.public_keys = {
      {"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
      {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}};

  EXPECT_EQ(expected_issuer, *issuer);
}

TEST_F(BatAdsIssuersUtilTest, DoNotGetIssuersForMissingType) {
  // Arrange
  const IssuersInfo issuers =
      BuildIssuers(7'200'000,
                   {{"JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=", 0.0},
                    {"crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=", 0.0}},
                   {});

  // Act
  const absl::optional<IssuerInfo> issuer =
      GetIssuerForType(issuers, IssuerType::kPayments);

  // Assert
  EXPECT_FALSE(issuer);
}

TEST_F(BatAdsIssuersUtilTest, IsIssuersValid) {
  // Arrange
  const IssuersInfo issuers =
      BuildIssuers(7'200'000,
                   {{"JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=", 0.0},
                    {"crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=", 0.0}},
                   {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
                    {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1},
                    {"XovQyvVWM8ez0mAzTtfqgPIbSpH5/idv8w0KJxhirwA=", 0.1},
                    {"wAcnJtb34Asykf+2jrTWrjFiaTqilklZ6bxLyR3LyFo=", 0.1},
                    {"ZvzeYOT1geUQXfOsYXBxZj/H26IfiBUVodHl51j68xI=", 0.1},
                    {"JlOezORiqLkFkvapoNRGWcMH3/g09/7M2UPEwMjRpFE=", 0.1},
                    {"hJP1nDjTdHcVDw347oH0XO+XBPPh5wZA2xWZE8QUSSA=", 0.1}});

  // Act
  const bool is_valid = IsIssuersValid(issuers);

  // Assert
  EXPECT_TRUE(is_valid);
}

TEST_F(BatAdsIssuersUtilTest, IsIssuersInvalid) {
  // Arrange
  const IssuersInfo issuers =
      BuildIssuers(7'200'000,
                   {{"JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=", 0.0},
                    {"crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=", 0.0}},
                   {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
                    {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1},
                    {"XovQyvVWM8ez0mAzTtfqgPIbSpH5/idv8w0KJxhirwA=", 0.1},
                    {"wAcnJtb34Asykf+2jrTWrjFiaTqilklZ6bxLyR3LyFo=", 0.1},
                    {"ZvzeYOT1geUQXfOsYXBxZj/H26IfiBUVodHl51j68xI=", 0.1},
                    {"JlOezORiqLkFkvapoNRGWcMH3/g09/7M2UPEwMjRpFE=", 0.1},
                    {"hJP1nDjTdHcVDw347oH0XO+XBPPh5wZA2xWZE8QUSSA=", 0.1},
                    {"+iyhYDv7W6cuFAD1tzsJIEQKEStTX9B/Tt62tqt+tG0=", 0.1},
                    {"oMx8hW6w8w/AUGoJMAMbrdeV1FoqrRkoR3BVa+5lDDk=", 0.1}});

  // Act
  const bool is_valid = IsIssuersValid(issuers);

  // Assert
  EXPECT_FALSE(is_valid);
}

}  // namespace ads
