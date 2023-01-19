/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_CONTENT_VALUE_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_CONTENT_VALUE_UTIL_H_

#include "base/values.h"

namespace ads {

struct AdContentInfo;

base::Value::Dict AdContentToValue(const AdContentInfo& ad_content);
AdContentInfo AdContentFromValue(const base::Value::Dict& root);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_CONTENT_VALUE_UTIL_H_
