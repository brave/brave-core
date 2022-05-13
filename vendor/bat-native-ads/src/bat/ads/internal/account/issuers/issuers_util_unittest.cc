/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/issuers/issuers_util.h"

#include "bat/ads/internal/account/issuers/issuers_info.h"
#include "bat/ads/internal/account/issuers/issuers_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsIssuersUtilTest : public UnitTestBase {
 protected:
  BatAdsIssuersUtilTest() = default;

  ~BatAdsIssuersUtilTest() override = default;
};

TEST_F(BatAdsIssuersUtilTest, HasIssuersChanged) {
  // Arrange
  BuildAndSetIssuers();

  // Act
  const IssuersInfo& issuers =
      BuildIssuers(3600000,
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
  const IssuersInfo& issuers =
      BuildIssuers(7200000,
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
  const IssuersInfo& issuers =
      BuildIssuers(7200000, {},
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
  const IssuersInfo& issuers =
      BuildIssuers(7200000,
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
  const IssuersInfo& issuers =
      BuildIssuers(7200000,
                   {{"JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=", 0.0},
                    {"crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=", 0.0}},
                   {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
                    {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}});

  // Act
  const absl::optional<IssuerInfo>& issuer_optional =
      GetIssuerForType(issuers, IssuerType::kPayments);
  ASSERT_NE(absl::nullopt, issuer_optional);

  const IssuerInfo& issuer = issuer_optional.value();

  // Assert
  IssuerInfo expected_issuer;
  expected_issuer.type = IssuerType::kPayments;
  expected_issuer.public_keys = {
      {"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
      {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}};

  EXPECT_EQ(expected_issuer, issuer);
}

TEST_F(BatAdsIssuersUtilTest, DoNotGetIssuersForMissingType) {
  // Arrange
  const IssuersInfo& issuers =
      BuildIssuers(7200000,
                   {{"JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=", 0.0},
                    {"crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=", 0.0}},
                   {});

  // Act
  const absl::optional<IssuerInfo>& issuer =
      GetIssuerForType(issuers, IssuerType::kPayments);

  // Assert
  EXPECT_EQ(absl::nullopt, issuer);
}

TEST_F(BatAdsIssuersUtilTest, IsIssuerValid) {
  // Arrange
  const IssuersInfo& issuers =
      BuildIssuers(7200000,
                   {{"JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=", 0.0},
                    {"crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=", 0.0}},
                   {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
                    {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1},
                    {"XovQyvVWM8ez0mAzTtfqgPIbSpH5/idv8w0KJxhirwA=", 0.1}});

  const absl::optional<IssuerInfo>& issuer_optional =
      GetIssuerForType(issuers, IssuerType::kPayments);
  ASSERT_NE(absl::nullopt, issuer_optional);

  const IssuerInfo& issuer = issuer_optional.value();

  // Act
  const bool is_valid = IsIssuerValid(issuer);

  // Assert
  EXPECT_TRUE(is_valid);
}

TEST_F(BatAdsIssuersUtilTest, IsIssuerInvalid) {
  // Arrange
  const IssuersInfo& issuers =
      BuildIssuers(7200000,
                   {{"JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=", 0.0},
                    {"crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=", 0.0}},
                   {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
                    {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1},
                    {"XovQyvVWM8ez0mAzTtfqgPIbSpH5/idv8w0KJxhirwA=", 0.1},
                    {"wAcnJtb34Asykf+2jrTWrjFiaTqilklZ6bxLyR3LyFo=", 0.1},
                    {"ZvzeYOT1geUQXfOsYXBxZj/H26IfiBUVodHl51j68xI=", 0.1}});

  const absl::optional<IssuerInfo>& issuer_optional =
      GetIssuerForType(issuers, IssuerType::kPayments);
  ASSERT_NE(absl::nullopt, issuer_optional);

  const IssuerInfo& issuer = issuer_optional.value();

  // Act
  const bool is_valid = IsIssuerValid(issuer);

  // Assert
  EXPECT_FALSE(is_valid);
}

TEST_F(BatAdsIssuersUtilTest, GetSmallestNonZeroDenominationForIssuerType) {
  // Arrange
  BuildAndSetIssuers();

  // Act
  const absl::optional<double> smallest_denomination_optional =
      GetSmallestNonZeroDenominationForIssuerType(IssuerType::kPayments);
  ASSERT_TRUE(smallest_denomination_optional);

  const double smallest_denomination = smallest_denomination_optional.value();

  // Assert
  EXPECT_EQ(0.1, smallest_denomination);
}

TEST_F(BatAdsIssuersUtilTest, GetSmallestDenominationForMissingIssuerType) {
  // Arrange

  // Act
  const absl::optional<double> smallest_denomination_optional =
      GetSmallestNonZeroDenominationForIssuerType(IssuerType::kPayments);

  // Assert
  EXPECT_FALSE(smallest_denomination_optional);
}

}  // namespace ads
