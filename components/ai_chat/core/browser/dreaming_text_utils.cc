// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/dreaming_text_utils.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/i18n/break_iterator.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/re2/src/re2/re2.h"

namespace ai_chat {

namespace {

// A phone number, or a card or account number with separators, has at least
// this count of digits.
constexpr size_t kMinDigitsInNumberSpan = 9;

const re2::RE2& EmailPattern() {
  static const base::NoDestructor<re2::RE2> pattern(
      R"([A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,})");
  return *pattern;
}

// An ID or an account number: 7 or more digits without a separator.
const re2::RE2& LongDigitRunPattern() {
  static const base::NoDestructor<re2::RE2> pattern(R"(\d{7,})");
  return *pattern;
}

// Digits with separators between them, for example "+1 (555) 123-4567".
const re2::RE2& NumberSpanPattern() {
  static const base::NoDestructor<re2::RE2> pattern(R"((\d[\d ().+-]*\d))");
  return *pattern;
}

// The ASCII words and digit runs of |text|, in lower case.
std::vector<std::string> Tokens(std::string_view text) {
  std::vector<std::string> tokens;
  std::string token;
  for (char c : text) {
    if (base::IsAsciiAlphaNumeric(c) &&
        (token.empty() ||
         base::IsAsciiDigit(c) == base::IsAsciiDigit(token[0]))) {
      token.push_back(c);
      continue;
    }
    if (!token.empty()) {
      tokens.push_back(std::move(token));
      token.clear();
    }
    if (base::IsAsciiAlphaNumeric(c)) {
      token.push_back(c);
    }
  }
  if (!token.empty()) {
    tokens.push_back(std::move(token));
  }
  return tokens;
}

constexpr const char* kDateWords[] = {
    "january",  "february", "march",    "april",     "may",
    "june",     "july",     "august",   "september", "october",
    "november", "december", "monday",   "tuesday",   "wednesday",
    "thursday", "friday",   "saturday", "sunday"};

}  // namespace

bool IsFaithfulRewrite(std::string_view text,
                       const std::vector<std::string>& sources,
                       const std::vector<base::Time>& dates) {
  if (text.empty() || base::UTF8ToUTF16(text).size() > kMaxMemoryTextLength) {
    return false;
  }
  base::flat_set<std::string> known;
  for (const auto& source : sources) {
    for (auto& token : Tokens(source)) {
      known.insert(base::ToLowerASCII(token));
    }
  }
  for (const char* word : kDateWords) {
    known.insert(word);
  }
  for (const auto& date : dates) {
    base::Time::Exploded exploded;
    date.LocalExplode(&exploded);
    for (int number : {exploded.year, exploded.month, exploded.day_of_month}) {
      known.insert(base::NumberToString(number));
      known.insert(base::StringPrintf("%02d", number));
    }
  }
  std::vector<std::string> tokens = Tokens(text);
  for (size_t i = 0; i < tokens.size(); ++i) {
    const std::string& token = tokens[i];
    const bool is_number = base::IsAsciiDigit(token[0]);
    const bool is_name = i > 0 && base::IsAsciiUpper(token[0]) && token != "I";
    if ((is_number || is_name) && !known.contains(base::ToLowerASCII(token))) {
      return false;
    }
  }
  return true;
}

std::vector<std::string> SplitIntoSentences(std::string_view text) {
  std::vector<std::string> sentences;
  const std::u16string text16 = base::UTF8ToUTF16(text);
  base::i18n::BreakIterator iter(text16,
                                 base::i18n::BreakIterator::BREAK_SENTENCE);
  if (!iter.Init()) {
    std::u16string_view trimmed =
        base::TrimWhitespace(std::u16string_view(text16), base::TRIM_ALL);
    if (!trimmed.empty()) {
      sentences.push_back(base::UTF16ToUTF8(trimmed));
    }
    return sentences;
  }
  while (iter.Advance()) {
    std::u16string_view sentence =
        base::TrimWhitespace(iter.GetString(), base::TRIM_ALL);
    if (!sentence.empty()) {
      sentences.push_back(base::UTF16ToUTF8(sentence));
    }
  }
  return sentences;
}

bool HasDeniedPattern(std::string_view sentence) {
  if (re2::RE2::PartialMatch(sentence, EmailPattern()) ||
      re2::RE2::PartialMatch(sentence, LongDigitRunPattern())) {
    return true;
  }
  std::string_view input = sentence;
  std::string_view span;
  while (re2::RE2::FindAndConsume(&input, NumberSpanPattern(), &span)) {
    size_t digits = 0;
    for (char c : span) {
      if (base::IsAsciiDigit(c)) {
        ++digits;
      }
    }
    if (digits >= kMinDigitsInNumberSpan) {
      return true;
    }
  }
  return false;
}

}  // namespace ai_chat
