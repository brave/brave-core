/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_DATABASE_DATABASE_TRANSACTION_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_DATABASE_DATABASE_TRANSACTION_UTIL_H_

#include "bat/ads/ads_client_callback.h"
#include "bat/ads/public/interfaces/ads.mojom-forward.h"

namespace ads::database {

void OnResultCallback(ResultCallback callback,
                      mojom::DBCommandResponseInfoPtr response);

}  // namespace ads::database

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_DATABASE_DATABASE_TRANSACTION_UTIL_H_
