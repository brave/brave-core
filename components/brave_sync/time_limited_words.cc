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
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_sync/crypto/crypto.h"
#include "brave/vendor/bip39wally-core-native/include/wally_bip39.h"
#include "brave/vendor/bip39wally-core-native/src/wordlist.h"

namespace brave_sync {

namespace {

// TODO(alexeybarabash): subject to change
static constexpr char kWordsv1SunsetDate[] = "Wed, 1 Jun 2022 00:00:00 GMT";

static constexpr char kWordsv2Epoch[] = "Fri, 15 Apr 2022 00:00:00 GMT";

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

std::string TimeLimitedWords::GenerateForNow(const std::string& pure_words) {
  return TimeLimitedWords::GenerateForDate(pure_words, Time::Now());
}

std::string TimeLimitedWords::GenerateForDate(const std::string& pure_words,
                                              const Time& not_after) {
  int days_since_words_v2_epoch =
      GetRoundedDaysDiff(GetWordsV2Epoch(), not_after);

  if (days_since_words_v2_epoch < 0) {
    // Something goes bad, requested |not_after| is even before sync v2 epoch
    return std::string();
  }

  std::string last_word = GetWordByIndex(days_since_words_v2_epoch);

  std::string time_limited_code = pure_words + " " + last_word;
  return time_limited_code;
}

WordsValidationResult TimeLimitedWords::Validate(
    const std::string& time_limited_words,
    std::string* pure_words) {
  CHECK_NE(pure_words, nullptr);
  *pure_words = std::string();

  static constexpr size_t kPureWordsCount = 24u;
  static constexpr size_t kWordsV2Count = 25u;

  auto now = Time::Now();

  std::vector<std::string> words = base::SplitString(
      time_limited_words, " ", base::WhitespaceHandling::TRIM_WHITESPACE,
      base::SplitResult::SPLIT_WANT_NONEMPTY);

  size_t num_words = words.size();

  if (num_words == kPureWordsCount) {
    if (now < GetWordsV1SunsetDay()) {
      std::string recombined_pure_words = base::JoinString(
          base::span<std::string>(words.begin(), kPureWordsCount), " ");
      if (crypto::IsPassphraseValid(recombined_pure_words)) {
        *pure_words = recombined_pure_words;
        return WordsValidationResult::kValid;
      } else {
        return WordsValidationResult::kNotValidPureWords;
      }
    } else {
      return WordsValidationResult::kVersionDeprecated;
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
        *pure_words = recombined_pure_words;
        return WordsValidationResult::kValid;
      } else if (days_actual > days_encoded) {
        return WordsValidationResult::kExpired;
      } else if (days_encoded > days_actual) {
        return WordsValidationResult::kValidForTooLong;
      }
    } else {
      return WordsValidationResult::kNotValidPureWords;
    }
  } else {
    return WordsValidationResult::kWrongWordsNumber;
  }

  NOTREACHED();
  return WordsValidationResult::kNotValidPureWords;
}

}  // namespace brave_sync
