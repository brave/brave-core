/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_UNITTEST_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_UNITTEST_UTIL_H_

#include <string>

namespace ads {

class ConfirmationType;
struct ConfirmationInfo;

ConfirmationInfo BuildConfirmation(const std::string& id,
                                   const std::string& creative_instance_id,
                                   const ConfirmationType& type);

ConfirmationInfo BuildConfirmation(const std::string& creative_instance_id,
                                   const ConfirmationType& type);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_UNITTEST_UTIL_H_
