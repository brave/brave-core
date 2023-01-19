/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LEGACY_MIGRATION_CONFIRMATIONS_LEGACY_CONFIRMATION_MIGRATION_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LEGACY_MIGRATION_CONFIRMATIONS_LEGACY_CONFIRMATION_MIGRATION_H_

#include "bat/ads/ads_callback.h"

namespace ads::confirmations {

void Migrate(InitializeCallback callback);

}  // namespace ads::confirmations

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LEGACY_MIGRATION_CONFIRMATIONS_LEGACY_CONFIRMATION_MIGRATION_H_
