/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_USER_DATA_CONVERSION_USER_DATA_BUILDER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_USER_DATA_CONVERSION_USER_DATA_BUILDER_H_

#include <functional>
#include <string>

namespace base {
class Value;
}  // namespace base

namespace ads {
namespace user_data {
namespace builder {

using BuildConversionCallback = std::function<void(base::Value)>;

void BuildConversion(const std::string& creative_instance_id,
                     BuildConversionCallback callback);

}  // namespace builder
}  // namespace user_data
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_USER_DATA_CONVERSION_USER_DATA_BUILDER_H_
