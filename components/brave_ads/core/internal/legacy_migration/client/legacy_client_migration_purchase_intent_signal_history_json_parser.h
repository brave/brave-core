/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_CLIENT_LEGACY_CLIENT_MIGRATION_PURCHASE_INTENT_SIGNAL_HISTORY_JSON_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_CLIENT_LEGACY_CLIENT_MIGRATION_PURCHASE_INTENT_SIGNAL_HISTORY_JSON_PARSER_H_

#include <optional>
#include <string_view>

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_info.h"

namespace brave_ads::json::reader {

// Parses purchase intent signal history from the legacy `client.json`
// `purchaseIntentSignalHistory` dictionary. Returns `std::nullopt` if `json`
// is malformed.
std::optional<PurchaseIntentSignalHistoryMap> ParsePurchaseIntentSignalHistory(
    std::string_view json);

}  // namespace brave_ads::json::reader

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_CLIENT_LEGACY_CLIENT_MIGRATION_PURCHASE_INTENT_SIGNAL_HISTORY_JSON_PARSER_H_
