/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_CONFIRMATIONS_LEGACY_CONFIRMATION_MIGRATION_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_CONFIRMATIONS_LEGACY_CONFIRMATION_MIGRATION_H_

#include <optional>

#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads {

struct WalletInfo;

// Parses `kConfirmationsJsonFilename`, writes valid confirmation queue items,
// confirmation tokens, and payment tokens into their respective database
// tables, then deletes the file. If `wallet` is absent only the confirmation
// queue is migrated. If the file does not exist the migration is considered
// complete.
void MigrateConfirmationState(std::optional<WalletInfo> wallet,
                              ResultCallback callback);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_CONFIRMATIONS_LEGACY_CONFIRMATION_MIGRATION_H_
