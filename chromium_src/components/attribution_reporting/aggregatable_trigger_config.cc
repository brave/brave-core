/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/attribution_reporting/aggregatable_trigger_config.h"

#include <optional>

#define Create Create_Unused
#include "src/components/attribution_reporting/aggregatable_trigger_config.cc"
#undef Create

namespace attribution_reporting {

std::optional<AggregatableTriggerConfig> AggregatableTriggerConfig::Create(
    SourceRegistrationTimeConfig source_registration_time_config,
    std::optional<std::string> trigger_context_id,
    AggregatableFilteringIdsMaxBytes max_bytes) {
  return std::nullopt;
}

}  // namespace attribution_reporting
