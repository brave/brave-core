/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_USER_DATA_ROTATING_HASH_USER_DATA_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_USER_DATA_ROTATING_HASH_USER_DATA_H_

#include <string>

namespace base {
class DictionaryValue;
}  // namespace base

namespace ads {
namespace user_data {

base::DictionaryValue GetRotatingHash(const std::string& creative_instance_id);

}  // namespace user_data
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_USER_DATA_ROTATING_HASH_USER_DATA_H_
