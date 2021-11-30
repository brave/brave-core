/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/qr_code_validator.h"

#include <memory>
#include <string>

#include "base/logging.h"
#include "base/strings/stringize_macros.h"
#include "base/time/time_override.h"
#include "brave/components/brave_sync/qr_code_data.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::subtle::ScopedTimeClockOverrides;

// Valid seed hex string for tests
#define SEED_HEX \
  "AABBCCFC6C2AC10EA018E7DA5AEC4D34707F3022CDBC3B209D1EE233498AC06B"
#define SEED_HEX_QUOTES "\"" SEED_HEX "\""
#define SEED_HEX_TOO_SHORT_QUOTES \
  "\"AABBCCFC6C2AC10EA018E7DA5AEC4D34707F3022CDBC3B209D1EE233498A\""
#define SEED_HEX_TOO_LONG_QUOTES \
  "\"" SEED_HEX                  \
  "AB"                           \
  "\""
#define SEED_HEX_WRONG_STRING_QUOTES \
  "\"" SEED_HEX                      \
  "Z"                                \
  "\""

// A date around which most the tests are made
#define NOT_AFTER 1637080050
#define NOT_AFTER_STR STRINGIZE(NOT_AFTER)
#define NOT_AFTER_STR_QUOTES "\"" NOT_AFTER_STR "\""

namespace brave_sync {

namespace {

base::Time::Exploded UTCExplode(const base::Time& time) {
  base::Time::Exploded exploded;
  time.UTCExplode(&exploded);
  return exploded;
}

base::Time g_overridden_now;
std::unique_ptr<ScopedTimeClockOverrides> OverrideWithTimeNow(
    const base::Time& overridden_now) {
  g_overridden_now = overridden_now;
  return std::make_unique<ScopedTimeClockOverrides>(
      []() { return g_overridden_now; }, nullptr, nullptr);
}

}  // namespace

TEST(QrCodeDataValidator, ValidData) {
  auto qr_data = QrCodeData::CreateWithActualDate(SEED_HEX);
  QrCodeDataValidationResult validationResult =
      QrCodeDataValidator::ValidateQrDataJson(qr_data->ToJson());
  EXPECT_EQ(validationResult, QrCodeDataValidationResult::kValid);
}

TEST(QrCodeDataValidator, QRv1SunsetDay) {
  EXPECT_FALSE(QrCodeDataValidator::GetQRv1SunsetDay().is_null());
  base::Time::Exploded exploded = {0};
  QrCodeDataValidator::GetQRv1SunsetDay().UTCExplode(&exploded);
  EXPECT_EQ(exploded.year, 2022);
  EXPECT_EQ(exploded.month, 1);
  EXPECT_EQ(exploded.day_of_month, 1);
  EXPECT_EQ(exploded.hour, 0);
  EXPECT_EQ(exploded.minute, 0);
  EXPECT_EQ(exploded.second, 0);
  EXPECT_EQ(exploded.millisecond, 0);
}

TEST(QrCodeDataValidator, VersionDeprecatedBeforeSunset) {
  auto time_override = OverrideWithTimeNow(
      QrCodeDataValidator::GetQRv1SunsetDay() - base::Days(1));
  QrCodeDataValidationResult verdict =
      QrCodeDataValidator::ValidateQrDataJson(SEED_HEX);
  EXPECT_EQ(verdict, QrCodeDataValidationResult::kValid);
}

TEST(QrCodeDataValidator, VersionDeprecatedAfterSunset) {
  auto time_override = OverrideWithTimeNow(
      QrCodeDataValidator::GetQRv1SunsetDay() + base::Days(1));
  QrCodeDataValidationResult verdict =
      QrCodeDataValidator::ValidateQrDataJson(SEED_HEX);
  EXPECT_EQ(verdict, QrCodeDataValidationResult::kVersionDeprecated);
}

TEST(QrCodeDataValidator, NotWellFormed) {
  // These tests are run around NOT_AFTER date to keep easily readed json
  // strings

  // Verify that NOT_AFTER looks like valid
  base::Time not_after_macro_time = QrCodeData::FromEpochSeconds(NOT_AFTER);
  ASSERT_GE(UTCExplode(not_after_macro_time).year, 2021);
  ASSERT_LT(not_after_macro_time, QrCodeDataValidator::GetQRv1SunsetDay());

  // Advance test device time 10 minutes backwards NOT_AFTER to get ourselfs
  // at the valid time iterval
  auto time_override =
      OverrideWithTimeNow(not_after_macro_time - base::Minutes(10));

  QrCodeDataValidationResult verdict;

  // clang-format off
  std::string version_too_small =
      "{ \"version\"       : \"1\","
      "  \"sync_code_hex\" : " SEED_HEX_QUOTES ","
      "  \"not_after\"     : " NOT_AFTER_STR_QUOTES "}";
  verdict = QrCodeDataValidator::ValidateQrDataJson(version_too_small);
  EXPECT_EQ(verdict, QrCodeDataValidationResult::kVersionDeprecated);

  std::string version_too_new =
      "{ \"version\"       : \"3\","
      "  \"sync_code_hex\" : " SEED_HEX_QUOTES ","
      "  \"not_after\"     : " NOT_AFTER_STR_QUOTES "}";
  verdict = QrCodeDataValidator::ValidateQrDataJson(version_too_new);
  EXPECT_EQ(verdict, QrCodeDataValidationResult::kVersionNotRecognized);

  std::string sync_code_hex_too_short =
      "{ \"version\"       : \"2\","
      "  \"sync_code_hex\" : " SEED_HEX_TOO_SHORT_QUOTES ","
      "  \"not_after\"     : " NOT_AFTER_STR_QUOTES "}";
  verdict = QrCodeDataValidator::ValidateQrDataJson(sync_code_hex_too_short);
  EXPECT_EQ(verdict, QrCodeDataValidationResult::kNotWellFormed);

  std::string sync_code_hex_too_long =
      "{ \"version\"       : \"2\","
      "  \"sync_code_hex\" : " SEED_HEX_TOO_LONG_QUOTES ","
      "  \"not_after\"     : " NOT_AFTER_STR_QUOTES "}";
  verdict = QrCodeDataValidator::ValidateQrDataJson(sync_code_hex_too_long);
  EXPECT_EQ(verdict, QrCodeDataValidationResult::kNotWellFormed);

  std::string wrong_hex_string =
      "{ \"version\"       : \"2\","
      "  \"sync_code_hex\" : " SEED_HEX_WRONG_STRING_QUOTES ","
      "  \"not_after\"     : " NOT_AFTER_STR_QUOTES "}";
  verdict = QrCodeDataValidator::ValidateQrDataJson(wrong_hex_string);
  EXPECT_EQ(verdict, QrCodeDataValidationResult::kNotWellFormed);

  std::string missing_sync_code_hex =
      "{ \"version\"       : \"2\","
      "  \"not_after\"     : \"" NOT_AFTER_STR "\" }";
  verdict = QrCodeDataValidator::ValidateQrDataJson(missing_sync_code_hex);
  EXPECT_EQ(verdict, QrCodeDataValidationResult::kNotWellFormed);

  std::string missing_version =
      "{ \"sync_code_hex\" : " SEED_HEX_QUOTES ","
      "  \"not_after\"     : " NOT_AFTER_STR_QUOTES "}";
  verdict = QrCodeDataValidator::ValidateQrDataJson(missing_version);
  EXPECT_EQ(verdict, QrCodeDataValidationResult::kNotWellFormed);

  std::string missing_expire =
      "{ \"version\"       : \"2\","
      "  \"sync_code_hex\" : " SEED_HEX_QUOTES "}";
  verdict = QrCodeDataValidator::ValidateQrDataJson(missing_expire);
  EXPECT_EQ(verdict, QrCodeDataValidationResult::kNotWellFormed);
  // clang-format on
}

TEST(QrCodeDataValidator, Expired) {
  auto time_override = OverrideWithTimeNow(
      QrCodeData::FromEpochSeconds(NOT_AFTER) + base::Minutes(10));

  // clang-format off
  std::string expired =
      "{ \"version\"       : \"2\","
      "  \"sync_code_hex\" : " SEED_HEX_QUOTES ","
      "  \"not_after\"     : " NOT_AFTER_STR_QUOTES "}";
  QrCodeDataValidationResult verdict =
      QrCodeDataValidator::ValidateQrDataJson(expired);
  EXPECT_EQ(verdict, QrCodeDataValidationResult::kExpired);
  // clang-format on
}

TEST(QrCodeDataValidator, ValidForTooLong) {
  auto time_override = OverrideWithTimeNow(
      QrCodeData::FromEpochSeconds(NOT_AFTER) - base::Minutes(61));
  // clang-format off
  std::string valid_for_too_long =
      "{ \"version\"       : \"2\","
      "  \"sync_code_hex\" : " SEED_HEX_QUOTES ","
      "  \"not_after\"     : " NOT_AFTER_STR_QUOTES "}";
  QrCodeDataValidationResult verdict =
      QrCodeDataValidator::ValidateQrDataJson(valid_for_too_long);
  EXPECT_EQ(verdict, QrCodeDataValidationResult::kValidForTooLong);
  // clang-format on
}

// Cross-timezone tests are not implemented, they supposed to be mostly about
// localtime <=> UTC convertions.

}  // namespace brave_sync
