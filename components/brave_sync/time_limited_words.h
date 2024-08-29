/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_TIME_LIMITED_WORDS_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_TIME_LIMITED_WORDS_H_

#include <map>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/time/time.h"
#include "base/types/expected.h"

namespace brave_sync {

FORWARD_DECLARE_TEST(TimeLimitedWordsTest, GenerateForDate);
FORWARD_DECLARE_TEST(TimeLimitedWordsTest, GetIndexByWord);
FORWARD_DECLARE_TEST(TimeLimitedWordsTest, GetRoundedDaysDiff);
FORWARD_DECLARE_TEST(TimeLimitedWordsTest, GetWordByIndex);
FORWARD_DECLARE_TEST(TimeLimitedWordsTest, Parse);
FORWARD_DECLARE_TEST(TimeLimitedWordsTest, GetNotAfter);

class TimeLimitedWords {
 public:
  enum class ValidationStatus {
    kValid = 0,  // for iOS and Android compatibility
    kNotValidPureWords = 1,
    kVersionDeprecated = 2,
    kExpired = 3,
    kValidForTooLong = 4,
    kWrongWordsNumber = 5,
  };

  enum class GenerateResult {
    kEmptyPureWords = 1,
    kNotAfterEarlierThanEpoch = 2,
  };

  static base::expected<std::string, GenerateResult> GenerateForNow(
      const std::string& pure_words);
  static base::expected<std::string, ValidationStatus> Parse(
      const std::string& time_limited_words);

  // The same as |Parse| but never give |kExpired| and |kValidForTooLong|
  // statuses
  static base::expected<std::string, ValidationStatus> ParseIgnoreDate(
      const std::string& time_limited_words);

  static std::string GenerateResultToText(
      const GenerateResult& generate_result);

  static base::Time GetNotAfter(const std::string& time_limited_words);

  static int GetWordsCount(const std::string& time_limited_words);

 private:
  FRIEND_TEST_ALL_PREFIXES(TimeLimitedWordsTest, GenerateForDate);
  FRIEND_TEST_ALL_PREFIXES(TimeLimitedWordsTest, GetIndexByWord);
  FRIEND_TEST_ALL_PREFIXES(TimeLimitedWordsTest, GetRoundedDaysDiff);
  FRIEND_TEST_ALL_PREFIXES(TimeLimitedWordsTest, GetWordByIndex);
  FRIEND_TEST_ALL_PREFIXES(TimeLimitedWordsTest, Parse);
  FRIEND_TEST_ALL_PREFIXES(TimeLimitedWordsTest, ParseIgnoreDate);
  FRIEND_TEST_ALL_PREFIXES(TimeLimitedWordsTest, GetNotAfter);

  enum class WrongDateBehaviour { kIgnore = 1, kDontAllow = 2 };
  static base::expected<std::string, ValidationStatus> ParseImpl(
      const std::string& time_limited_words,
      WrongDateBehaviour wrong_date_behaviour);

  static base::Time GetWordsV1SunsetDay();
  static base::Time GetWordsV2Epoch();

  static base::expected<std::string, GenerateResult> GenerateForDate(
      const std::string& pure_words,
      const base::Time& not_after);

  static int GetRoundedDaysDiff(const base::Time& time1,
                                const base::Time& time2);

  static std::string GetWordByIndex(size_t index);
  static int GetIndexByWord(const std::string& word);

  static base::Time words_v1_sunset_day_;
  static base::Time words_v2_epoch_;
};

}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_TIME_LIMITED_WORDS_H_
