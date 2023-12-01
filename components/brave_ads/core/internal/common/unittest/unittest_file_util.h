/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_FILE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_FILE_UTIL_H_

#include <optional>
#include <string>


namespace brave_ads {

std::optional<std::string> MaybeReadFileToString(const std::string& name);

std::optional<std::string> MaybeReadFileToStringAndReplaceTags(
    const std::string& name);

std::optional<std::string> MaybeReadDataResourceToString(
    const std::string& name);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_FILE_UTIL_H_
