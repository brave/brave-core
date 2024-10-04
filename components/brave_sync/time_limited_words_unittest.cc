/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/time_limited_words.h"

#include <memory>
#include <utility>

#include "base/strings/strcat.h"
#include "base/time/time_override.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::subtle::ScopedTimeClockOverrides;

namespace brave_sync {

namespace {

constexpr char kValidSyncCode[] =
    "fringe digital begin feed equal output proof cheap "
    "exotic ill sure question trial squirrel glove celery "
    "awkward push jelly logic broccoli almost grocery drift";

constexpr char kInvalidSyncCode[] =
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
  auto generate_result = TimeLimitedWords::GenerateForDate(
      kValidSyncCode, TimeLimitedWords::GetWordsV2Epoch());
  ASSERT_TRUE(generate_result.has_value());
  EXPECT_EQ(base::StrCat({kValidSyncCode, " abandon"}),
            generate_result.value());

  generate_result = TimeLimitedWords::GenerateForDate(
      kValidSyncCode, TimeLimitedWords::GetWordsV2Epoch() + base::Days(1));
  ASSERT_TRUE(generate_result.has_value());
  EXPECT_EQ(base::StrCat({kValidSyncCode, " ability"}),
            generate_result.value());

  generate_result = TimeLimitedWords::GenerateForDate(
      kValidSyncCode, TimeLimitedWords::GetWordsV2Epoch() - base::Days(1));
  ASSERT_FALSE(generate_result.has_value());
  EXPECT_EQ(generate_result.error(),
            TimeLimitedWords::GenerateResult::kNotAfterEarlierThanEpoch);

  generate_result = TimeLimitedWords::GenerateForNow("");
  ASSERT_FALSE(generate_result.has_value());
  EXPECT_EQ(generate_result.error(),
            TimeLimitedWords::GenerateResult::kEmptyPureWords);
}

TEST(TimeLimitedWordsTest, Parse) {
  using ValidationStatus = TimeLimitedWords::ValidationStatus;
  std::string pure_words;
  base::expected<std::string, ValidationStatus> pure_words_with_status;
  {
    // Valid v1 sync code, prior to sunset date
    auto time_override = OverrideWithTimeNow(
        TimeLimitedWords::GetWordsV1SunsetDay() - base::Days(1));
    pure_words_with_status = TimeLimitedWords::Parse(kValidSyncCode);
    EXPECT_TRUE(pure_words_with_status.has_value());
    EXPECT_EQ(pure_words_with_status.value(), kValidSyncCode);
  }

  {
    // Valid v1 sync code plus ending space, prior to sunset date
    auto time_override = OverrideWithTimeNow(
        TimeLimitedWords::GetWordsV1SunsetDay() - base::Days(1));
    pure_words_with_status =
        TimeLimitedWords::Parse(base::StrCat({kValidSyncCode, " "}));
    EXPECT_TRUE(pure_words_with_status.has_value());
    EXPECT_EQ(pure_words_with_status.value(), kValidSyncCode);
  }

  {
    // Invalid v1 sync code, prior to sunset date
    auto time_override = OverrideWithTimeNow(
        TimeLimitedWords::GetWordsV1SunsetDay() - base::Days(1));
    pure_words_with_status = TimeLimitedWords::Parse(kInvalidSyncCode);
    EXPECT_FALSE(pure_words_with_status.has_value());
    EXPECT_EQ(pure_words_with_status.error(),
              ValidationStatus::kNotValidPureWords);
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
    EXPECT_TRUE(pure_words_with_status.has_value());
    EXPECT_EQ(pure_words_with_status.value(), kValidSyncCode);
  }

  {
    // Valid v2 sync code, after sunset date, expired
    const std::string valid25thExpiredWord =
        TimeLimitedWords::GetWordByIndex(15);
    const std::string valid25thExpiredWords =
        base::StrCat({kValidSyncCode, " ", valid25thExpiredWord});

    auto time_override = OverrideWithTimeNow(anchorDayForWordsV2);
    pure_words_with_status = TimeLimitedWords::Parse(valid25thExpiredWords);
    EXPECT_FALSE(pure_words_with_status.has_value());
    EXPECT_EQ(pure_words_with_status.error(), ValidationStatus::kExpired);
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
    EXPECT_FALSE(pure_words_with_status.has_value());
    EXPECT_EQ(pure_words_with_status.error(),
              ValidationStatus::kValidForTooLong);
  }

  {
    // Wrong words number
    auto time_override = OverrideWithTimeNow(anchorDayForWordsV2);
    pure_words_with_status = TimeLimitedWords::Parse("abandon ability");
    EXPECT_FALSE(pure_words_with_status.has_value());
    EXPECT_EQ(pure_words_with_status.error(),
              ValidationStatus::kWrongWordsNumber);

    pure_words_with_status = TimeLimitedWords::Parse(
        base::StrCat({valid25thAnchoredWords, " abandon ability"}));
    EXPECT_FALSE(pure_words_with_status.has_value());
    EXPECT_EQ(pure_words_with_status.error(),
              ValidationStatus::kWrongWordsNumber);
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
    EXPECT_TRUE(pure_words_with_status.has_value());
    EXPECT_EQ(pure_words_with_status.value(), kValidSyncCode);
  }
}

TEST(TimeLimitedWordsTest, ParseIgnoreDate) {
  using ValidationStatus = TimeLimitedWords::ValidationStatus;
  base::expected<std::string, ValidationStatus> pure_words_with_status;

  const base::Time anchorDayForWordsV2 =
      TimeLimitedWords::GetWordsV2Epoch() + base::Days(20);

  {
    // Valid v2 sync code, after sunset date, expired, but pure words should be
    // extracted
    const std::string valid25thExpiredWord =
        TimeLimitedWords::GetWordByIndex(15);
    const std::string valid25thExpiredWords =
        base::StrCat({kValidSyncCode, " ", valid25thExpiredWord});

    auto time_override = OverrideWithTimeNow(anchorDayForWordsV2);
    pure_words_with_status =
        TimeLimitedWords::ParseIgnoreDate(valid25thExpiredWords);
    EXPECT_TRUE(pure_words_with_status.has_value());
    EXPECT_EQ(pure_words_with_status.value(), kValidSyncCode);
  }

  {
    // Valid v2 sync code, after sunset date, valid for too long, but pure words
    // should be extracted
    const std::string valid25thValidTooLongWord =
        TimeLimitedWords::GetWordByIndex(25);
    const std::string valid25thValidTooLongWords =
        base::StrCat({kValidSyncCode, " ", valid25thValidTooLongWord});

    auto time_override = OverrideWithTimeNow(anchorDayForWordsV2);
    pure_words_with_status =
        TimeLimitedWords::ParseIgnoreDate(valid25thValidTooLongWords);
    EXPECT_TRUE(pure_words_with_status.has_value());
    EXPECT_EQ(pure_words_with_status.value(), kValidSyncCode);
  }
}

TEST(TimeLimitedWordsTest, GetNotAfter) {
  const base::Time anchorDayForWordsV2 =
      TimeLimitedWords::GetWordsV2Epoch() + base::Days(20);

  const auto& generate_result =
      TimeLimitedWords::GenerateForDate(kValidSyncCode, anchorDayForWordsV2);
  EXPECT_TRUE(generate_result.has_value());

  base::Time not_after = TimeLimitedWords::GetNotAfter(generate_result.value());

  // We must able to connect before |not_after| and we must be rejected after
  // |not_after|
  std::pair<int, bool> hours_to_result_data[] = {
      {-12, true}, {-1, true}, {1, false}};

  for (const auto& hours_to_result : hours_to_result_data) {
    auto time_override =
        OverrideWithTimeNow(not_after + base::Hours(hours_to_result.first));
    auto parse_status = TimeLimitedWords::Parse(generate_result.value());
    EXPECT_EQ(parse_status.has_value(), hours_to_result.second);
  }
}

TEST(TimeLimitedWordsTest, GetNotAfterForPureWords) {
  EXPECT_EQ(TimeLimitedWords::GetNotAfter(kValidSyncCode), base::Time());
}

TEST(TimeLimitedWordsTest, GetWordsCount) {
  // Normal flow
  EXPECT_EQ(TimeLimitedWords::GetWordsCount(kValidSyncCode), 24);
  // Empty case
  EXPECT_EQ(TimeLimitedWords::GetWordsCount(""), 0);
  EXPECT_EQ(TimeLimitedWords::GetWordsCount(" \n\t"), 0);
  // STR from desktop issue
  EXPECT_EQ(TimeLimitedWords::GetWordsCount("d  d"), 2);
  // Previously failed case
  EXPECT_EQ(TimeLimitedWords::GetWordsCount("a\nb\tc"), 3);
  // Additional leading and trailing whitespaces
  EXPECT_EQ(TimeLimitedWords::GetWordsCount(" \n \t A   B \t"), 2);
}

}  // namespace brave_sync
