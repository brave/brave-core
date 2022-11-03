/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_ELIGIBLE_ADS_CALLBACK_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_ELIGIBLE_ADS_CALLBACK_H_

#include <functional>

#include "base/callback.h"

namespace ads {

template <typename T>
using GetEligibleAdsCallback = std::function<void(const bool, const T&)>;

template <typename T>
using GetEligibleAdsOnceCallback =
    base::OnceCallback<void(const bool, const T&)>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_ELIGIBLE_ADS_CALLBACK_H_
