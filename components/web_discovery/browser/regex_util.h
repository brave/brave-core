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
#include "third_party/re2/src/re2/re2.h"

namespace web_discovery {

class RegexUtil {
 public:
  RegexUtil();
  ~RegexUtil();

  RegexUtil(const RegexUtil&) = delete;
  RegexUtil& operator=(const RegexUtil&) = delete;

  bool CheckForEmail(const std::string_view str);
  bool CheckForLongNumber(const std::string_view str, size_t max_length);
  bool CheckPathAndQueryStringKeywords(const std::string_view path_and_query);
  bool CheckQueryStringOrRefKeywords(const std::string_view str);
  bool CheckQueryHTTPCredentials(const std::string_view str);
  void RemovePunctuation(std::string& str);
  void TransformToAlphanumeric(std::string& str);

 private:
  std::optional<re2::RE2> email_regex_;
  // key is long number map length
  base::flat_map<size_t, std::unique_ptr<re2::RE2>> long_number_regexes_;
  std::deque<re2::RE2> path_and_query_string_keyword_regexes_;
  std::deque<re2::RE2> query_string_and_ref_keyword_regexes_;
  std::optional<re2::RE2> http_password_regex_;
  std::optional<re2::RE2> punctuation_regex_;
  std::optional<re2::RE2> non_alphanumeric_regex_;
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_REGEX_UTIL_H_
