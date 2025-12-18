/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_LOCALE_LANGUAGE_CODE_H_
#define BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_LOCALE_LANGUAGE_CODE_H_

#include <optional>
#include <string>

namespace metrics {

// Return the current language of the device as string.
std::optional<std::string> MaybeGetLanguageCodeString();

}  // namespace metrics

#endif  // BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_LOCALE_LANGUAGE_CODE_H_
