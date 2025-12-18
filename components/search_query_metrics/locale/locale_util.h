/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_LOCALE_LOCALE_UTIL_H_
#define BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_LOCALE_LOCALE_UTIL_H_

#include <string>

namespace metrics {

inline constexpr char kDefaultLanguageCode[] = "en";

// Retrieves the current language code. The result of the first call is cached
// in a static variable. If the language code is changed, the application must
// be restarted to ensure the new value is applied.
const std::string& CurrentLanguageCode();

// For testing purposes only. Allows you to set a new value for the current
// language code. The new value will be used in subsequent `CurrentLanguageCode`
// calls.
std::string& MutableCurrentLanguageCodeForTesting();

}  // namespace metrics

#endif  // BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_LOCALE_LOCALE_UTIL_H_
