/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_ATTRIBUTION_REPORTING_AGGREGATABLE_TRIGGER_CONFIG_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_ATTRIBUTION_REPORTING_AGGREGATABLE_TRIGGER_CONFIG_H_

#include "components/attribution_reporting/aggregatable_filtering_id_max_bytes.h"
#include "components/attribution_reporting/source_registration_time_config.mojom.h"
#include "components/attribution_reporting/trigger_registration_error.mojom-forward.h"

#define Create(...)           \
  Create_Unused(__VA_ARGS__); \
  static std::optional<AggregatableTriggerConfig> Create(__VA_ARGS__)

#include "src/components/attribution_reporting/aggregatable_trigger_config.h"  // IWYU pragma: export

#undef Create

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_ATTRIBUTION_REPORTING_AGGREGATABLE_TRIGGER_CONFIG_H_
