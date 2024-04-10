/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "components/attribution_reporting/aggregatable_utils.h"

#define GetNullAggregatableReports GetNullAggregatableReports_ChromiumImpl
#include "src/components/attribution_reporting/aggregatable_utils.cc"
#undef GetNullAggregatableReports

namespace attribution_reporting {

std::vector<NullAggregatableReport> GetNullAggregatableReports(
    const AggregatableTriggerConfig&,
    base::Time trigger_time,
    std::optional<base::Time> attributed_source_time,
    GenerateNullAggregatableReportFunc) {
  return {};
}

}  // namespace attribution_reporting
