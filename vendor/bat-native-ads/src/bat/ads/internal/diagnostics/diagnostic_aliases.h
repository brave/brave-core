/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_ALIASES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_ALIASES_H_

#include <memory>

#include "base/containers/flat_map.h"
#include "bat/ads/internal/diagnostics/diagnostic_entry_interface.h"
#include "bat/ads/internal/diagnostics/diagnostic_entry_types.h"

namespace ads {

using DiagnosticMap = base::flat_map<DiagnosticEntryType,
                                     std::unique_ptr<DiagnosticEntryInterface>>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_ALIASES_H_
