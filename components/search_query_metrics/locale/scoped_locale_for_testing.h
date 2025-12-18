/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_LOCALE_SCOPED_LOCALE_FOR_TESTING_H_
#define BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_LOCALE_SCOPED_LOCALE_FOR_TESTING_H_

#include <string>

namespace metrics::test {

// Set the current language code for testing, restoring the original language
// code upon destruction.
class ScopedCurrentLanguageCode final {
 public:
  explicit ScopedCurrentLanguageCode(const std::string& language_code);

  ScopedCurrentLanguageCode(const ScopedCurrentLanguageCode& other) = delete;
  ScopedCurrentLanguageCode& operator=(const ScopedCurrentLanguageCode& other) =
      delete;

  ~ScopedCurrentLanguageCode();

  // Resets the language code to the original value.
  void Reset();

  // Sets the language code to the specified value.
  void Set(const std::string& language_code);

 private:
  std::string last_language_code_;
};

}  // namespace metrics::test

#endif  // BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_LOCALE_SCOPED_LOCALE_FOR_TESTING_H_
