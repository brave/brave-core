/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_VALUE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_VALUE_UTIL_H_

#include "base/functional/callback_forward.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_types.h"

namespace brave_ads {

// Builds the `name`/`value` list for every registered entry in `diagnostics`
// for which `should_include` returns `true`.
base::ListValue DiagnosticsToList(
    const DiagnosticMap& diagnostics,
    const base::RepeatingCallback<bool(DiagnosticEntryType)>& should_include);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_VALUE_UTIL_H_
