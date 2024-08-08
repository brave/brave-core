/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_FILE_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_FILE_TEST_UTIL_H_

#include <optional>
#include <string>

namespace brave_ads::test {

std::optional<std::string> MaybeReadFileToString(const std::string& name);

// The file can include `<time:period>` tags for mocking timestamps. `period`
// can be one of the following: `now`, `distant_past`, `distant_future`, `+/-#
// seconds`, `+/-# minutes`, `+/-# hours`, or `+/-# days`, i.e.,
//
//     `<time:+7 days>`
std::optional<std::string> MaybeReadFileToStringAndReplaceTags(
    const std::string& name);

// Read bundled .pak data resources.
std::optional<std::string> MaybeReadDataResourceToString(
    const std::string& name);

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_FILE_TEST_UTIL_H_
