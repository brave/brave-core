/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_DATABASE_DATABASE_UTIL_H_
#define BAT_ADS_INTERNAL_DATABASE_DATABASE_UTIL_H_

#include "bat/ads/ads_client.h"
#include "bat/ads/mojom.h"

namespace ads {
namespace database {

void OnResultCallback(
    DBCommandResponsePtr response,
    ResultCallback callback);

}  // namespace database
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_DATABASE_DATABASE_UTIL_H_
