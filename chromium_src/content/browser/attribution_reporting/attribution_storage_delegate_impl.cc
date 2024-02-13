/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "content/browser/attribution_reporting/attribution_storage_delegate_impl.h"

#define GetNullAggregatableReports GetNullAggregatableReports_ChromiumImpl
#include "src/content/browser/attribution_reporting/attribution_storage_delegate_impl.cc"
#undef GetNullAggregatableReports

namespace content {

std::vector<AttributionStorageDelegate::NullAggregatableReport>
AttributionStorageDelegateImpl::GetNullAggregatableReports(
    const AttributionTrigger& trigger,
    base::Time trigger_time,
    std::optional<base::Time> attributed_source_time) const {
  return {};
}

}  // namespace content
