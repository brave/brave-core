/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_ADS_HISTORY_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_ADS_HISTORY_UTIL_H_

#include <string>

namespace ads {

class ConfirmationType;
struct AdHistoryInfo;
struct AdInfo;

AdHistoryInfo BuildAdHistory(const AdInfo& ad,
                             const ConfirmationType& confirmation_type,
                             const std::string& title,
                             const std::string& body);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_ADS_HISTORY_UTIL_H_
