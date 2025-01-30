/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_JSON_READER_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_JSON_READER_TEST_UTIL_H_

#include <optional>
#include <string>

#include "base/values.h"

namespace brave_ads::test {

std::optional<base::Value::Dict> ReadDictionaryFromJsonString(
    const std::string& json);

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_JSON_READER_TEST_UTIL_H_
