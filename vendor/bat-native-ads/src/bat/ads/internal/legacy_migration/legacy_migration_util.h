/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_LEGACY_MIGRATION_LEGACY_MIGRATION_UTIL_H_
#define BAT_ADS_INTERNAL_LEGACY_MIGRATION_LEGACY_MIGRATION_UTIL_H_

#include <stdint.h>

namespace ads {

uint64_t MigrateTimestampToDoubleT(
    const uint64_t timestamp_in_seconds);

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_LEGACY_MIGRATION_LEGACY_MIGRATION_UTIL_H_
