// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/web_discovery/browser/regex_util.h"

#include <array>

#include "base/strings/string_util.h"

namespace {

constexpr char kEmailRegex[] =
    "[a-z0-9\\-_@]+(@|%40|%(25)+40)[a-z0-9\\-_]+\\.[a-z0-9\\-_]";
constexpr char kHttpPasswordRegex[] = "[^:]+:[^@]+@";
constexpr char kEuroLongWordPatternRegex[] = "^[a-zA-ZäöüéÄÖÜ][a-zäöüéß]+$";
constexpr char kWhitespaceRegex[] = "\\s+";
constexpr char kISSNRegex[] = "([0-9]{4}-?[0-9]{3}[0-9xX])";
constexpr char kNumberFragmentRegex[] = "([^\\p{L}\\s]+)";
constexpr char kNonDigitRegex[] = "[^0-9]";
constexpr char kSafeUrlParameterRegex[] = "^[a-z-_]{1,18}$";

constexpr std::array<std::string_view, 6> kMiscPrivateUrlCheckRegexes = {
    "(?i)[&?]redirect(?:-?url)?=",
    "(?i)[&?#/=;](?:http|https)(?:[/]|%3A%2F)",
    "(?i)[/]order[/].",
    "(?i)[/]auth[/]realms[/]",
    "(?i)[/]protocol[/]openid-connect[/]",
    "(?i)((maps|route[^r-]).*|@)\\d{1,2}[^\\d]-?\\d{6}.+\\d{1,2}[^\\d]-?\\d{"
    "6}"};

}  // anonymous namespace

namespace web_discovery {

RegexUtil::RegexUtil() = default;
RegexUtil::~RegexUtil() = default;

RegexUtil* RegexUtil::GetInstance() {
  static base::NoDestructor<RegexUtil> regex_util;
  return regex_util.get();
}

bool RegexUtil::CheckForEmail(std::string_view str) {
  if (!email_regex_) {
    email_regex_.emplace(kEmailRegex);
  }
  return re2::RE2::PartialMatch(str, *email_regex_);
}

bool RegexUtil::CheckQueryHTTPCredentials(std::string_view str) {
  if (!http_password_regex_) {
    http_password_regex_.emplace(kHttpPasswordRegex);
  }
  return re2::RE2::PartialMatch(str, *http_password_regex_);
}

bool RegexUtil::CheckForEuroLongWord(std::string_view str) {
  if (!long_word_regex_) {
    long_word_regex_.emplace(kEuroLongWordPatternRegex);
  }
  return re2::RE2::FullMatch(str, *long_word_regex_);
}

std::string RegexUtil::NormalizeWhitespace(std::string_view str) {
  if (!whitespace_regex_) {
    whitespace_regex_.emplace(kWhitespaceRegex);
  }
  std::string result(str);
  re2::RE2::GlobalReplace(&result, *whitespace_regex_, " ");
  return result;
}

bool RegexUtil::FindAndConsumeISSN(std::string_view* input,
                                   std::string* match) {
  if (!issn_regex_) {
    issn_regex_.emplace(kISSNRegex);
  }
  return re2::RE2::FindAndConsume(input, *issn_regex_, match);
}

bool RegexUtil::FindAndConsumeNumberFragment(std::string_view* input,
                                             std::string* match) {
  if (!number_fragment_regex_) {
    number_fragment_regex_.emplace(kNumberFragmentRegex);
  }
  if (!non_digit_regex_) {
    non_digit_regex_.emplace(kNonDigitRegex);
  }

  std::string raw_fragment;
  if (!re2::RE2::FindAndConsume(input, *number_fragment_regex_,
                                &raw_fragment)) {
    return false;
  }
  // Filter out non-digit characters using GlobalReplace
  *match = raw_fragment;
  re2::RE2::GlobalReplace(match, *non_digit_regex_, "");
  return true;
}

bool RegexUtil::CheckForMiscPrivateUrls(std::string_view str) {
  if (misc_private_url_regexes_.empty()) {
    for (const auto& regex_str : kMiscPrivateUrlCheckRegexes) {
      misc_private_url_regexes_.emplace_back(regex_str);
    }
  }
  for (const auto& regex : misc_private_url_regexes_) {
    if (re2::RE2::PartialMatch(str, regex)) {
      return true;
    }
  }
  return false;
}

bool RegexUtil::CheckForSafeUrlParameter(std::string_view value) {
  if (!safe_url_parameter_regex_) {
    safe_url_parameter_regex_.emplace(kSafeUrlParameterRegex);
  }
  return re2::RE2::FullMatch(value, *safe_url_parameter_regex_);
}

}  // namespace web_discovery
