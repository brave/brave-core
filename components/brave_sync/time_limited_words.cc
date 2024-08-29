/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/time_limited_words.h"

#include <cmath>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "brave/components/brave_sync/crypto/crypto.h"
#include "brave/third_party/bip39wally-core-native/include/wally_bip39.h"
#include "brave/third_party/bip39wally-core-native/src/wordlist.h"

namespace brave_sync {

namespace {

static constexpr char kWordsv1SunsetDate[] = "Mon, 1 Aug 2022 00:00:00 GMT";
static constexpr char kWordsv2Epoch[] = "Tue, 10 May 2022 00:00:00 GMT";

static constexpr size_t kWordsV2Count = 25u;

std::vector<std::string> SplitWords(const std::string& words_string) {
  return base::SplitString(words_string, " \n\t",
                           base::WhitespaceHandling::TRIM_WHITESPACE,
                           base::SplitResult::SPLIT_WANT_NONEMPTY);
}

}  // namespace

using base::Time;
using base::TimeDelta;

Time TimeLimitedWords::words_v1_sunset_day_;
Time TimeLimitedWords::words_v2_epoch_;

std::string TimeLimitedWords::GetWordByIndex(size_t index) {
  DCHECK_EQ(BIP39_WORDLIST_LEN, 2048);
  index = index % BIP39_WORDLIST_LEN;
  char* word = nullptr;
  if (bip39_get_word(nullptr, index, &word) != WALLY_OK) {
    LOG(ERROR) << "bip39_get_word failed for index " << index;
    return std::string();
  }

  std::string str_word = word;
  wally_free_string(word);

  return str_word;
}

int TimeLimitedWords::GetIndexByWord(const std::string& word) {
  std::string word_prepared = base::ToLowerASCII(word);

  struct words* mnemonic_w = nullptr;
  if (bip39_get_wordlist(nullptr, &mnemonic_w) != WALLY_OK) {
    DCHECK(false);
    return -1;
  }

  DCHECK_NE(mnemonic_w, nullptr);
  size_t idx = wordlist_lookup_word(mnemonic_w, word_prepared.c_str());
  if (!idx) {
    return -1;
  }

  return idx - 1;
}

Time TimeLimitedWords::GetWordsV1SunsetDay() {
  if (words_v1_sunset_day_.is_null()) {
    bool convert_result =
        Time::FromUTCString(kWordsv1SunsetDate, &words_v1_sunset_day_);
    CHECK(convert_result);
  }

  CHECK(!words_v1_sunset_day_.is_null());

  return words_v1_sunset_day_;
}

Time TimeLimitedWords::GetWordsV2Epoch() {
  if (words_v2_epoch_.is_null()) {
    bool convert_result = Time::FromUTCString(kWordsv2Epoch, &words_v2_epoch_);
    CHECK(convert_result);
  }

  CHECK(!words_v2_epoch_.is_null());

  return words_v2_epoch_;
}

int TimeLimitedWords::GetRoundedDaysDiff(const Time& time1, const Time& time2) {
  TimeDelta delta = time2 - time1;

  double delta_in_days_f = delta.InMillisecondsF() / Time::kMillisecondsPerDay;

  int days_rounded = std::round(delta_in_days_f);
  return days_rounded;
}

base::expected<std::string, TimeLimitedWords::GenerateResult>
TimeLimitedWords::GenerateForNow(const std::string& pure_words) {
  return TimeLimitedWords::GenerateForDate(pure_words, Time::Now());
}

base::expected<std::string, TimeLimitedWords::GenerateResult>
TimeLimitedWords::GenerateForDate(const std::string& pure_words,
                                  const Time& not_after) {
  using GenerateResult = TimeLimitedWords::GenerateResult;
  if (pure_words.empty()) {
    // Most likely we could not get access to the keychain on macOS or Linux
    // and could not decrypt and provide the correct pure words
    return base::unexpected(GenerateResult::kEmptyPureWords);
  }

  int days_since_words_v2_epoch =
      GetRoundedDaysDiff(GetWordsV2Epoch(), not_after);

  if (days_since_words_v2_epoch < 0) {
    // Something goes bad, requested |not_after| is even earlier than sync v2
    // epoch
    return base::unexpected(GenerateResult::kNotAfterEarlierThanEpoch);
  }

  std::string last_word = GetWordByIndex(days_since_words_v2_epoch);

  std::string time_limited_code = base::StrCat({pure_words, " ", last_word});
  return time_limited_code;
}

base::expected<std::string, TimeLimitedWords::ValidationStatus>
TimeLimitedWords::ParseImpl(const std::string& time_limited_words,
                            WrongDateBehaviour wrong_date_behaviour) {
  using ValidationStatus = TimeLimitedWords::ValidationStatus;

  static constexpr size_t kPureWordsCount = 24u;

  auto now = Time::Now();

  std::vector<std::string> words = SplitWords(time_limited_words);

  size_t num_words = words.size();

  if (num_words == kPureWordsCount) {
    if (now < GetWordsV1SunsetDay()) {
      std::string recombined_pure_words = base::JoinString(
          base::span<std::string>(words.begin(), kPureWordsCount), " ");
      if (crypto::IsPassphraseValid(recombined_pure_words)) {
        return recombined_pure_words;
      } else {
        return base::unexpected(ValidationStatus::kNotValidPureWords);
      }
    } else {
      return base::unexpected(ValidationStatus::kVersionDeprecated);
    }
  } else if (num_words == kWordsV2Count) {
    std::string recombined_pure_words = base::JoinString(
        base::span<std::string>(words.begin(), kPureWordsCount), " ");
    if (crypto::IsPassphraseValid(recombined_pure_words)) {
      int days_actual =
          GetRoundedDaysDiff(GetWordsV2Epoch(), now) % BIP39_WORDLIST_LEN;

      int days_encoded = GetIndexByWord(words[kWordsV2Count - 1]);
      DCHECK(days_encoded < BIP39_WORDLIST_LEN);

      int days_abs_diff = std::abs(days_actual - days_encoded);
      if (days_abs_diff <= 1) {
        return recombined_pure_words;
      } else if (wrong_date_behaviour == WrongDateBehaviour::kIgnore) {
        return recombined_pure_words;
      } else if (days_actual > days_encoded) {
        return base::unexpected(ValidationStatus::kExpired);
      } else if (days_encoded > days_actual) {
        return base::unexpected(ValidationStatus::kValidForTooLong);
      }
    } else {
      return base::unexpected(ValidationStatus::kNotValidPureWords);
    }
  } else {
    return base::unexpected(ValidationStatus::kWrongWordsNumber);
  }

  NOTREACHED_IN_MIGRATION();
  return base::unexpected(ValidationStatus::kNotValidPureWords);
}

base::expected<std::string, TimeLimitedWords::ValidationStatus>
TimeLimitedWords::Parse(const std::string& time_limited_words) {
  return ParseImpl(time_limited_words, WrongDateBehaviour::kDontAllow);
}

base::expected<std::string, TimeLimitedWords::ValidationStatus>
TimeLimitedWords::ParseIgnoreDate(const std::string& time_limited_words) {
  return ParseImpl(time_limited_words, WrongDateBehaviour::kIgnore);
}

std::string TimeLimitedWords::GenerateResultToText(
    const GenerateResult& generate_result) {
  switch (generate_result) {
    case TimeLimitedWords::GenerateResult::kEmptyPureWords:
      return "Input pure words are empty";
    case TimeLimitedWords::GenerateResult::kNotAfterEarlierThanEpoch:
      return "Requested not_after is earlier than sync words v2 epoch";
  }
}

// static
base::Time TimeLimitedWords::GetNotAfter(
    const std::string& time_limited_words) {
  std::vector<std::string> words = SplitWords(time_limited_words);
  size_t num_words = words.size();
  if (num_words != kWordsV2Count) {
    return base::Time();
  }

  int days_encoded = GetIndexByWord(words[kWordsV2Count - 1u]);
  const base::Time anchor_time = GetWordsV2Epoch() + base::Days(days_encoded);

  // We need to find not_after as the offset from the anchor time which would
  // satisfy this pseudo equation derived from TimeLimitedWords::ParseImpl:
  //
  //    GetRoundedDaysDiff(anchor + x, anchor) = 2
  //        expand GetRoundedDaysDiff:
  //    round(anchor - (anchor + x)) = 2
  //    round(x) = 2
  //    x=1.5...2.49999
  //        and we need the smallest value of x, so it is 1.5 days or 36 hours.
  const base::TimeDelta k1_5dayOffset = base::Hours(36);
  base::Time not_after = anchor_time + k1_5dayOffset;

  // Re-check in debug build the solution is correct.
  // We should have two days rounded difference for our result, which means code
  // words are rejected. And a moment before our result diffecence should be 1,
  // which means code words are accepted.
  DCHECK_EQ(GetRoundedDaysDiff(anchor_time, not_after), 2);
  DCHECK_EQ(GetRoundedDaysDiff(anchor_time, not_after - base::Seconds(1)), 1);

  return not_after;
}

// static
int TimeLimitedWords::GetWordsCount(const std::string& time_limited_words) {
  return SplitWords(time_limited_words).size();
}

}  // namespace brave_sync
