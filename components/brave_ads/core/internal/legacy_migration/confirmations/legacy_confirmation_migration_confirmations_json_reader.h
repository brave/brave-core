/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_CONFIRMATIONS_LEGACY_CONFIRMATION_MIGRATION_CONFIRMATIONS_JSON_READER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_CONFIRMATIONS_LEGACY_CONFIRMATION_MIGRATION_CONFIRMATIONS_JSON_READER_H_

#include <optional>
#include <string>

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"

namespace brave_ads::json::reader {

std::optional<ConfirmationList> ReadConfirmations(const std::string& json);

}  // namespace brave_ads::json::reader

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_CONFIRMATIONS_LEGACY_CONFIRMATION_MIGRATION_CONFIRMATIONS_JSON_READER_H_
