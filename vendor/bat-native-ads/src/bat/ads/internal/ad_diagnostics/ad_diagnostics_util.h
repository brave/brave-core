/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DIAGNOSTICS_AD_DIAGNOSTICS_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DIAGNOSTICS_AD_DIAGNOSTICS_UTIL_H_

#include <string>

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class Time;
class Value;
}  // namespace base

namespace ads {

void AppendDiagnosticsKeyValue(const std::string& key,
                               const std::string& value,
                               base::Value* diagnostics);

absl::optional<std::string> GetDiagnosticsValueByKey(
    const base::Value& diagnostics,
    const std::string& key);

std::string ConvertToString(const bool value);

std::string ConvertToString(const base::Time& time);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DIAGNOSTICS_AD_DIAGNOSTICS_UTIL_H_
