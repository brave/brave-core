/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LEGACY_MIGRATION_CONVERSIONS_LEGACY_CONVERSIONS_MIGRATION_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LEGACY_MIGRATION_CONVERSIONS_LEGACY_CONVERSIONS_MIGRATION_H_

#include "bat/ads/ads_callback.h"

namespace ads::conversions {

void Migrate(InitializeCallback callback);

}  // namespace ads::conversions

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LEGACY_MIGRATION_CONVERSIONS_LEGACY_CONVERSIONS_MIGRATION_H_
