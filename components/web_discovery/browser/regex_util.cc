// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/web_discovery/browser/regex_util.h"

#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"

namespace {

constexpr char kLongNumberRegexPrefix[] = "[0-9]{";
constexpr char kLongNumberRegexSuffix[] = ",}";
constexpr char kEmailRegex[] =
    "[a-z0-9\\-_@]+(@|%40|%(25)+40)[a-z0-9\\-_]+\\.[a-z0-9\\-_]";
constexpr char kHttpPasswordRegex[] = "[^:]+:[^@]+@";
constexpr char kNotAlphanumericRegex[] = "[^a-zA-Z0-9]";
constexpr char kPunctuationRegex[] = "[!\"'()*,-./:;?[\\]^_`{|}~%$=&+#]";

constexpr std::array<std::string_view, 10> kPathAndQueryStringCheckRegexes = {
    "(?i)\\/admin([\\/\\?#=]|$)",
    "(?i)\\/wp-admin([\\/\\?#=]|$)",
    "(?i)\\/edit([\\/\\?#=]|$)",
    "(?i)[&\\?#\\/]share([\\/\\?#=]|$)",
    "(?i)[&\\?#\\/;]sharing([\\/\\?#=]|$)",
    "(?i)[&\\?#\\/;]logout([\\/\\?#=]|$)",
    "(?i)WebLogic",
    "(?i)[&\\?#\\/;]token([\\/\\?#=_;]|$)",
    "(?i)[&\\?#\\/;]trk([\\/\\?#=_]|$)",
    "[&\\?#\\/=;](http|https)(:\\/|\\%3A\\%2F)"};

constexpr std::array<std::string_view, 20> kQueryStringAndRefCheckRegexes = {
    "(?i)[&\\?#_\\-;]user",     "(?i)[&\\?#_\\-;]token",
    "(?i)[&\\?#_\\-;]auth",     "(?i)[&\\?#_\\-;]uid",
    "(?i)[&\\?#_\\-;]email",    "(?i)[&\\?#_\\-;]usr",
    "(?i)[&\\?#_\\-;]pin",      "(?i)[&\\?#_\\-;]pwd",
    "(?i)[&\\?#_\\-;]password", "(?i)[&\\?#;]u[=#]",
    "(?i)[&\\?#;]url[=#]",      "(?i)[&\\?#_\\-;]http",
    "(?i)[&\\?#_\\-;]ref[=#]",  "(?i)[&\\?#_\\-;]red[=#]",
    "(?i)[&\\?#_\\-;]trk",      "(?i)[&\\?#_\\-;]track",
    "(?i)[&\\?#_\\-;]shar",     "(?i)[&\\?#_\\-;]login",
    "(?i)[&\\?#_\\-;]logout",   "(?i)[&\\?#_\\-;]session",
};

}  // anonymous namespace

namespace web_discovery {

RegexUtil::RegexUtil() = default;
RegexUtil::~RegexUtil() = default;

bool RegexUtil::CheckForEmail(const std::string_view str) {
  if (!email_regex_) {
    email_regex_.emplace(kEmailRegex);
  }
  return re2::RE2::PartialMatch(str, *email_regex_);
}

bool RegexUtil::CheckForLongNumber(const std::string_view str,
                                   size_t max_length) {
  if (!long_number_regexes_.contains(max_length)) {
    auto regex_str = base::StrCat({kLongNumberRegexPrefix,
                                   base::NumberToString(max_length + 1),
                                   kLongNumberRegexSuffix});
    long_number_regexes_[max_length] = std::make_unique<re2::RE2>(regex_str);
  }
  return re2::RE2::PartialMatch(str, *long_number_regexes_[max_length]);
}

void RegexUtil::RemovePunctuation(std::string& str) {
  if (!punctuation_regex_) {
    punctuation_regex_.emplace(kPunctuationRegex);
  }
  re2::RE2::GlobalReplace(&str, *punctuation_regex_, "");
}

void RegexUtil::TransformToAlphanumeric(std::string& str) {
  if (!non_alphanumeric_regex_) {
    non_alphanumeric_regex_.emplace(kNotAlphanumericRegex);
  }
  re2::RE2::GlobalReplace(&str, *non_alphanumeric_regex_, "");
}

bool RegexUtil::CheckPathAndQueryStringKeywords(
    const std::string_view path_and_query) {
  if (path_and_query_string_keyword_regexes_.empty()) {
    for (const auto& regex_str : kPathAndQueryStringCheckRegexes) {
      path_and_query_string_keyword_regexes_.emplace_back(regex_str);
    }
  }
  for (const auto& regex : path_and_query_string_keyword_regexes_) {
    if (re2::RE2::PartialMatch(path_and_query, regex)) {
      return true;
    }
  }
  return false;
}

bool RegexUtil::CheckQueryStringOrRefKeywords(const std::string_view str) {
  if (query_string_and_ref_keyword_regexes_.empty()) {
    for (const auto& regex_str : kQueryStringAndRefCheckRegexes) {
      query_string_and_ref_keyword_regexes_.emplace_back(regex_str);
    }
  }
  for (const auto& regex : query_string_and_ref_keyword_regexes_) {
    if (re2::RE2::PartialMatch(str, regex)) {
      return true;
    }
  }
  return false;
}

bool RegexUtil::CheckQueryHTTPCredentials(const std::string_view str) {
  if (!http_password_regex_) {
    http_password_regex_.emplace(kHttpPasswordRegex);
  }
  return re2::RE2::PartialMatch(str, *http_password_regex_);
}

}  // namespace web_discovery
