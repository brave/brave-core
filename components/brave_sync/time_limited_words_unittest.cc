/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/time_limited_words.h"

#include <memory>

#include "base/logging.h"
#include "base/test/gtest_util.h"
#include "base/time/time_override.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::subtle::ScopedTimeClockOverrides;

namespace brave_sync {

namespace {

const char kValidSyncCode[] =
    "fringe digital begin feed equal output proof cheap "
    "exotic ill sure question trial squirrel glove celery "
    "awkward push jelly logic broccoli almost grocery drift";

const char kInvalidSyncCode[] =
    "fringe digital begin feed equal output proof cheap "
    "exotic ill sure question trial squirrel glove celery "
    "awkward push jelly logic broccoli almost grocery driftZ";

base::Time g_overridden_now;
std::unique_ptr<ScopedTimeClockOverrides> OverrideWithTimeNow(
    const base::Time& overridden_now) {
  g_overridden_now = overridden_now;
  return std::make_unique<ScopedTimeClockOverrides>(
      []() { return g_overridden_now; }, nullptr, nullptr);
}
}  // namespace

TEST(TimeLimitedWordsTest, GetRoundedDaysDiff) {
  const base::Time time1 = TimeLimitedWords::GetWordsV2Epoch();

  base::Time time2 = time1 + base::Hours(11);
  EXPECT_EQ(TimeLimitedWords::GetRoundedDaysDiff(time1, time2), 0);

  time2 = time1 + base::Hours(13);
  EXPECT_EQ(TimeLimitedWords::GetRoundedDaysDiff(time1, time2), 1);
  EXPECT_EQ(TimeLimitedWords::GetRoundedDaysDiff(time2, time1), -1);
}

TEST(TimeLimitedWordsTest, GetIndexByWord) {
  EXPECT_EQ(TimeLimitedWords::GetIndexByWord("abandon"), 0);
  EXPECT_EQ(TimeLimitedWords::GetIndexByWord("ability"), 1);
  EXPECT_EQ(TimeLimitedWords::GetIndexByWord("not_bip39_word"), -1);
}

TEST(TimeLimitedWordsTest, GetWordByIndex) {
  EXPECT_EQ(TimeLimitedWords::GetWordByIndex(0), "abandon");
  EXPECT_EQ(TimeLimitedWords::GetWordByIndex(1), "ability");
  EXPECT_EQ(TimeLimitedWords::GetWordByIndex(2047), "zoo");
  EXPECT_EQ(TimeLimitedWords::GetWordByIndex(2048), "abandon");
}

TEST(TimeLimitedWordsTest, GenerateForDate) {
  EXPECT_EQ(std::string(kValidSyncCode) + " abandon",
            TimeLimitedWords::GenerateForDate(
                kValidSyncCode, TimeLimitedWords::GetWordsV2Epoch()));
  EXPECT_EQ(
      std::string(kValidSyncCode) + " ability",
      TimeLimitedWords::GenerateForDate(
          kValidSyncCode, TimeLimitedWords::GetWordsV2Epoch() + base::Days(1)));
  EXPECT_EQ("", TimeLimitedWords::GenerateForDate(
                    kValidSyncCode,
                    TimeLimitedWords::GetWordsV2Epoch() - base::Days(1)));
}

TEST(TimeLimitedWordsTest, Validate) {
  std::string pure_words;
  WordsValidationResult result;

  {
    // Valid v1 sync code, prior to sunset date
    auto time_override = OverrideWithTimeNow(
        TimeLimitedWords::GetWordsV1SunsetDay() - base::Days(1));
    result = TimeLimitedWords::Validate(kValidSyncCode, &pure_words);
    EXPECT_EQ(result, WordsValidationResult::kValid);
    EXPECT_EQ(pure_words, kValidSyncCode);
  }

  {
    // Valid v1 sync code plus ending space, prior to sunset date
    auto time_override = OverrideWithTimeNow(
        TimeLimitedWords::GetWordsV1SunsetDay() - base::Days(1));
    result = TimeLimitedWords::Validate(kValidSyncCode + std::string(" "),
                                        &pure_words);
    EXPECT_EQ(result, WordsValidationResult::kValid);
    EXPECT_EQ(pure_words, kValidSyncCode);
  }

  {
    // Invalid v1 sync code, prior to sunset date
    auto time_override = OverrideWithTimeNow(
        TimeLimitedWords::GetWordsV1SunsetDay() - base::Days(1));
    result = TimeLimitedWords::Validate(kInvalidSyncCode, &pure_words);
    EXPECT_EQ(result, WordsValidationResult::kNotValidPureWords);
    EXPECT_EQ(pure_words, "");
  }

  const base::Time anchorDayForWordsV2 =
      TimeLimitedWords::GetWordsV2Epoch() + base::Days(20);
  const std::string valid25thAnchoredWord =
      TimeLimitedWords::GetWordByIndex(20);
  const std::string valid25thAnchoredWords =
      kValidSyncCode + std::string(" ") + valid25thAnchoredWord;

  {
    // Valid v2 sync code, after sunset date, around anchored day
    auto time_override = OverrideWithTimeNow(anchorDayForWordsV2);
    result = TimeLimitedWords::Validate(valid25thAnchoredWords, &pure_words);
    EXPECT_EQ(result, WordsValidationResult::kValid);
    EXPECT_EQ(pure_words, kValidSyncCode);
  }

  {
    // Valid v2 sync code, after sunset date, expired
    const std::string valid25thExpiredWord =
        TimeLimitedWords::GetWordByIndex(15);
    const std::string valid25thExpiredWords =
        kValidSyncCode + std::string(" ") + valid25thExpiredWord;

    auto time_override = OverrideWithTimeNow(anchorDayForWordsV2);
    result = TimeLimitedWords::Validate(valid25thExpiredWords, &pure_words);
    EXPECT_EQ(result, WordsValidationResult::kExpired);
    EXPECT_EQ(pure_words, "");
  }

  {
    // Valid v2 sync code, after sunset date, valid for too long
    const std::string valid25thValidTooLongWord =
        TimeLimitedWords::GetWordByIndex(25);
    const std::string valid25thValidTooLongWords =
        kValidSyncCode + std::string(" ") + valid25thValidTooLongWord;

    auto time_override = OverrideWithTimeNow(anchorDayForWordsV2);
    result =
        TimeLimitedWords::Validate(valid25thValidTooLongWords, &pure_words);
    EXPECT_EQ(result, WordsValidationResult::kValidForTooLong);
    EXPECT_EQ(pure_words, "");
  }

  {
    // Wrong words number
    auto time_override = OverrideWithTimeNow(anchorDayForWordsV2);
    result = TimeLimitedWords::Validate("abandon ability", &pure_words);
    EXPECT_EQ(result, WordsValidationResult::kWrongWordsNumber);
    EXPECT_EQ(pure_words, "");

    result = TimeLimitedWords::Validate(
        valid25thAnchoredWords + " abandon ability", &pure_words);
    EXPECT_EQ(result, WordsValidationResult::kWrongWordsNumber);
    EXPECT_EQ(pure_words, "");
  }

  {
    // Valid v2 sync code, after sunset date, day modulo 2048 which is
    // "2027-08-11 00:00:00.000 UTC"
    const std::string validModulo2048Word =
        TimeLimitedWords::GetWordByIndex(2048);
    const std::string validModulo2048Words =
        kValidSyncCode + std::string(" ") + validModulo2048Word;

    auto time_override = OverrideWithTimeNow(
        TimeLimitedWords::GetWordsV2Epoch() + base::Days(2048));
    result = TimeLimitedWords::Validate(validModulo2048Words, &pure_words);
    EXPECT_EQ(result, WordsValidationResult::kValid);
    EXPECT_EQ(pure_words, kValidSyncCode);
  }
}

TEST(TimeLimitedWordsDeathTest, ValidateCheckWithNullptr) {
  EXPECT_CHECK_DEATH(TimeLimitedWords::Validate("abandon ability", nullptr));
}

}  // namespace brave_sync
