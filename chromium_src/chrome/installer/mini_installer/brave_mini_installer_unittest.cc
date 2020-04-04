// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/mini_installer/mini_installer.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace mini_installer {

class BraveMiniInstallerTest: public testing::Test {
 public:
  BraveMiniInstallerTest() {
  }
  ~BraveMiniInstallerTest() override {}
};


TEST_F(BraveMiniInstallerTest, HasNoReferralCode) {
  ReferralCodeString referral_code;
  EXPECT_FALSE(ParseReferralCode(L"BraveBrowserSetup.exe", referral_code));
}

TEST_F(BraveMiniInstallerTest, HasStandardReferralCode) {
  ReferralCodeString referral_code;
  EXPECT_TRUE(ParseReferralCode(L"BraveBrowserSetup-FOO123.exe",
                          referral_code));
  EXPECT_STREQ(referral_code.get(), L"FOO123");
}

TEST_F(BraveMiniInstallerTest, HasStandardReferralCode) {
  ReferralCodeString referral_code;
  EXPECT_TRUE(ParseReferralCode(L"BraveBrowserSetup-foo123.exe",
                          referral_code));
  EXPECT_STREQ(referral_code.get(), L"FOO123");
}

TEST_F(BraveMiniInstallerTest, HasStandardReferralCodeWithPath) {
  ReferralCodeString referral_code;
  EXPECT_TRUE(ParseReferralCode(L"c:/foo/bar/BraveBrowserSetup-FOO123.exe",
                          referral_code));
  EXPECT_STREQ(referral_code.get(), L"FOO123");
}

TEST_F(BraveMiniInstallerTest,
                HasStandardReferralCodeWithDeduplicatingSuffix) {
  ReferralCodeString referral_code;
  EXPECT_TRUE(ParseReferralCode(L"c:/foo/bar/BraveBrowserSetup-FOO123 (1).exe",
                          referral_code));
  EXPECT_STREQ(referral_code.get(), L"FOO123");
}

TEST_F(BraveMiniInstallerTest,
                HasStandardReferralCodeWithDeduplicatingSuffixNoSpaces) {
  ReferralCodeString referral_code;
  EXPECT_TRUE(ParseReferralCode(L"c:/foo/bar/BraveBrowserSetup-FOO123(1).exe",
                          referral_code));
  EXPECT_STREQ(referral_code.get(), L"FOO123");
}

TEST_F(BraveMiniInstallerTest,
                HasStandardReferralCodeWithDeduplicatingSuffixExtraSpaces) {
  ReferralCodeString referral_code;
  EXPECT_TRUE(ParseReferralCode(
                          L"c:/foo/bar/BraveBrowserSetup-FOO123   (1).exe",
                          referral_code));
  EXPECT_STREQ(referral_code.get(), L"FOO123");
}

TEST_F(BraveMiniInstallerTest, HasInvalidStandardReferralCodeReversed) {
  ReferralCodeString referral_code;
  EXPECT_FALSE(ParseReferralCode(L"BraveBrowserSetup-123FOO.exe",
                          referral_code));
}

TEST_F(BraveMiniInstallerTest, HasInvalidStandardReferralCodeNoDigits) {
  ReferralCodeString referral_code;
  EXPECT_FALSE(ParseReferralCode(L"BraveBrowserSetup-FOO.exe", referral_code));
}

TEST_F(BraveMiniInstallerTest, HasInvalidStandardReferralCodeNoLetters) {
  ReferralCodeString referral_code;
  EXPECT_FALSE(ParseReferralCode(L"BraveBrowserSetup-123.exe", referral_code));
}

TEST_F(BraveMiniInstallerTest, HasInvalidStandardReferralCodeTooManyDigits) {
  ReferralCodeString referral_code;
  EXPECT_FALSE(ParseReferralCode(L"BraveBrowserSetup-FOO1234.exe",
                          referral_code));
}

TEST_F(BraveMiniInstallerTest, HasInvalidStandardReferralCodeTooFewDigits) {
  ReferralCodeString referral_code;
  EXPECT_FALSE(ParseReferralCode(L"BraveBrowserSetup-FOO12.exe",
                          referral_code));
}

TEST_F(BraveMiniInstallerTest, HasInvalidStandardReferralCodeTooManyLetters) {
  ReferralCodeString referral_code;
  EXPECT_FALSE(ParseReferralCode(L"BraveBrowserSetup-FOOO123.exe",
                          referral_code));
}

TEST_F(BraveMiniInstallerTest, HasInvalidStandardReferralCodeTooFewLetters) {
  ReferralCodeString referral_code;
  EXPECT_FALSE(ParseReferralCode(L"BraveBrowserSetup-FO123.exe",
                          referral_code));
}

TEST_F(BraveMiniInstallerTest, HasExtendedReferralCode) {
  ReferralCodeString referral_code;
  EXPECT_TRUE(ParseReferralCode(L"BraveBrowserSetup-extended-code.exe",
                          referral_code));
  EXPECT_STREQ(referral_code.get(), L"extended-code");
}

TEST_F(BraveMiniInstallerTest,
                HasInvalidExtendedReferralCodeNonAlphabeticCharacters) {
  ReferralCodeString referral_code;
  EXPECT_FALSE(ParseReferralCode(
                          L"BraveBrowserSetup-invalid-extended-c0de.exe",
                          referral_code));
}

TEST_F(BraveMiniInstallerTest, HasInvalidExtendedReferralCodeTooFewWords) {
  ReferralCodeString referral_code;
  EXPECT_FALSE(ParseReferralCode(L"BraveBrowserSetup-invalidextendedcode.exe",
                          referral_code));
}

}  // namespace mini_installer
