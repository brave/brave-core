/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_COVARIATE_LOGS_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_COVARIATE_LOGS_UTIL_H_

#include <cstdint>
#include <string>

namespace base {
class Time;
}  // namespace base

namespace ads {

std::string ToString(const int value);
std::string ToString(const int64_t value);
std::string ToString(const double value);
std::string ToString(const bool value);
std::string ToString(const base::Time time);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_COVARIATE_LOGS_UTIL_H_
