/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/time_limited_words.h"

#include <memory>

#include "base/strings/strcat.h"
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
  EXPECT_EQ(base::StrCat({kValidSyncCode, " abandon"}),
            TimeLimitedWords::GenerateForDate(
                kValidSyncCode, TimeLimitedWords::GetWordsV2Epoch()));
  EXPECT_EQ(
      base::StrCat({kValidSyncCode, " ability"}),
      TimeLimitedWords::GenerateForDate(
          kValidSyncCode, TimeLimitedWords::GetWordsV2Epoch() + base::Days(1)));
  EXPECT_EQ("", TimeLimitedWords::GenerateForDate(
                    kValidSyncCode,
                    TimeLimitedWords::GetWordsV2Epoch() - base::Days(1)));
}

TEST(TimeLimitedWordsTest, Parse) {
  std::string pure_words;
  TimeLimitedWords::PureWordsWithStatus pure_words_with_status;

  {
    // Valid v1 sync code, prior to sunset date
    auto time_override = OverrideWithTimeNow(
        TimeLimitedWords::GetWordsV1SunsetDay() - base::Days(1));
    pure_words_with_status = TimeLimitedWords::Parse(kValidSyncCode);
    EXPECT_EQ(pure_words_with_status.status, WordsValidationStatus::kValid);
    EXPECT_EQ(pure_words_with_status.pure_words.value(), kValidSyncCode);
  }

  {
    // Valid v1 sync code plus ending space, prior to sunset date
    auto time_override = OverrideWithTimeNow(
        TimeLimitedWords::GetWordsV1SunsetDay() - base::Days(1));
    pure_words_with_status =
        TimeLimitedWords::Parse(base::StrCat({kValidSyncCode, " "}));
    EXPECT_EQ(pure_words_with_status.status, WordsValidationStatus::kValid);
    EXPECT_EQ(pure_words_with_status.pure_words.value(), kValidSyncCode);
  }

  {
    // Invalid v1 sync code, prior to sunset date
    auto time_override = OverrideWithTimeNow(
        TimeLimitedWords::GetWordsV1SunsetDay() - base::Days(1));
    pure_words_with_status = TimeLimitedWords::Parse(kInvalidSyncCode);
    EXPECT_EQ(pure_words_with_status.status,
              WordsValidationStatus::kNotValidPureWords);
    EXPECT_FALSE(pure_words_with_status.pure_words.has_value());
  }

  const base::Time anchorDayForWordsV2 =
      TimeLimitedWords::GetWordsV2Epoch() + base::Days(20);
  const std::string valid25thAnchoredWord =
      TimeLimitedWords::GetWordByIndex(20);
  const std::string valid25thAnchoredWords =
      base::StrCat({kValidSyncCode, " ", valid25thAnchoredWord});

  {
    // Valid v2 sync code, after sunset date, around anchored day
    auto time_override = OverrideWithTimeNow(anchorDayForWordsV2);
    pure_words_with_status = TimeLimitedWords::Parse(valid25thAnchoredWords);
    EXPECT_EQ(pure_words_with_status.status, WordsValidationStatus::kValid);
    EXPECT_EQ(pure_words_with_status.pure_words.value(), kValidSyncCode);
  }

  {
    // Valid v2 sync code, after sunset date, expired
    const std::string valid25thExpiredWord =
        TimeLimitedWords::GetWordByIndex(15);
    const std::string valid25thExpiredWords =
        base::StrCat({kValidSyncCode, " ", valid25thExpiredWord});

    auto time_override = OverrideWithTimeNow(anchorDayForWordsV2);
    pure_words_with_status = TimeLimitedWords::Parse(valid25thExpiredWords);
    EXPECT_EQ(pure_words_with_status.status, WordsValidationStatus::kExpired);
    EXPECT_FALSE(pure_words_with_status.pure_words.has_value());
  }

  {
    // Valid v2 sync code, after sunset date, valid for too long
    const std::string valid25thValidTooLongWord =
        TimeLimitedWords::GetWordByIndex(25);
    const std::string valid25thValidTooLongWords =
        base::StrCat({kValidSyncCode, " ", valid25thValidTooLongWord});

    auto time_override = OverrideWithTimeNow(anchorDayForWordsV2);
    pure_words_with_status =
        TimeLimitedWords::Parse(valid25thValidTooLongWords);
    EXPECT_EQ(pure_words_with_status.status,
              WordsValidationStatus::kValidForTooLong);
    EXPECT_FALSE(pure_words_with_status.pure_words.has_value());
  }

  {
    // Wrong words number
    auto time_override = OverrideWithTimeNow(anchorDayForWordsV2);
    pure_words_with_status = TimeLimitedWords::Parse("abandon ability");
    EXPECT_EQ(pure_words_with_status.status,
              WordsValidationStatus::kWrongWordsNumber);
    EXPECT_FALSE(pure_words_with_status.pure_words.has_value());

    pure_words_with_status = TimeLimitedWords::Parse(
        base::StrCat({valid25thAnchoredWords, " abandon ability"}));
    EXPECT_EQ(pure_words_with_status.status,
              WordsValidationStatus::kWrongWordsNumber);
    EXPECT_FALSE(pure_words_with_status.pure_words.has_value());
  }

  {
    // Valid v2 sync code, after sunset date, day modulo 2048 which is
    // "2027-11-23 00:00:00.000 UTC"
    // Note: While this date is way too far into the future, the codes repeat
    // after a few years and so this becomes valid again, an unfortunate
    // limitation of this scheme.
    const std::string validModulo2048Word =
        TimeLimitedWords::GetWordByIndex(2048);
    const std::string validModulo2048Words =
        base::StrCat({kValidSyncCode, " ", validModulo2048Word});

    auto time_override = OverrideWithTimeNow(
        TimeLimitedWords::GetWordsV2Epoch() + base::Days(2048));
    pure_words_with_status = TimeLimitedWords::Parse(validModulo2048Words);
    EXPECT_EQ(pure_words_with_status.status, WordsValidationStatus::kValid);
    EXPECT_EQ(pure_words_with_status.pure_words.value(), kValidSyncCode);
  }
}

}  // namespace brave_sync
