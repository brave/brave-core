/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_REGEX_UTIL_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_REGEX_UTIL_H_

#include <deque>
#include <memory>
#include <optional>
#include <string>

#include "base/containers/flat_map.h"
#include "base/no_destructor.h"
#include "third_party/re2/src/re2/re2.h"

namespace web_discovery {

// Lazily creates and caches pre-compiled regexes, mainly used for
// privacy risk assessment of page URLs/contents.
// This class is not thread safe.
class RegexUtil {
 public:
  static RegexUtil* GetInstance();

  ~RegexUtil();

  RegexUtil(const RegexUtil&) = delete;
  RegexUtil& operator=(const RegexUtil&) = delete;

  bool CheckForEmail(std::string_view str);
  bool CheckQueryHTTPCredentials(std::string_view str);
  bool CheckForEuroLongWord(std::string_view str);
  bool FindAndConsumeISSN(std::string_view* input, std::string* match);
  bool FindAndConsumeNumberFragment(std::string_view* input,
                                    std::string* match);
  std::string NormalizeWhitespace(std::string_view str);
  bool CheckForMiscPrivateUrls(std::string_view str);
  bool CheckForSafeUrlParameter(std::string_view value);

 private:
  friend class base::NoDestructor<RegexUtil>;
  RegexUtil();

  std::optional<re2::RE2> email_regex_;
  // key is long number map length
  base::flat_map<size_t, std::unique_ptr<re2::RE2>> long_number_regexes_;
  std::deque<re2::RE2> misc_private_url_regexes_;
  std::optional<re2::RE2> http_password_regex_;
  std::optional<re2::RE2> non_alphanumeric_regex_;
  std::optional<re2::RE2> long_word_regex_;
  std::optional<re2::RE2> whitespace_regex_;
  std::optional<re2::RE2> issn_regex_;
  std::optional<re2::RE2> number_fragment_regex_;
  std::optional<re2::RE2> non_digit_regex_;
  std::optional<re2::RE2> safe_url_parameter_regex_;
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_REGEX_UTIL_H_
