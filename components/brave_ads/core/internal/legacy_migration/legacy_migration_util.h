/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_LEGACY_MIGRATION_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_LEGACY_MIGRATION_UTIL_H_

#include <string>

namespace brave_ads {

// Removes the file with the given `name` from persistent storage. Used to
// remove legacy JSON files after their contents have been migrated to the
// database.
void MaybeDeleteFile(const std::string& name);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_LEGACY_MIGRATION_UTIL_H_
