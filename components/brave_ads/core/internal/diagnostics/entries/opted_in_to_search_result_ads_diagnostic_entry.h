/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_OPTED_IN_TO_SEARCH_RESULT_ADS_DIAGNOSTIC_ENTRY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_OPTED_IN_TO_SEARCH_RESULT_ADS_DIAGNOSTIC_ENTRY_H_

#include <string>

#include "brave/components/brave_ads/core/internal/diagnostics/entries/diagnostic_entry_interface.h"

namespace brave_ads {

class OptedInToSearchResultAdsDiagnosticEntry final
    : public DiagnosticEntryInterface {
 public:
  // DiagnosticEntryInterface:
  DiagnosticEntryType GetType() const override;
  std::string GetName() const override;
  std::string GetValue() const override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_OPTED_IN_TO_SEARCH_RESULT_ADS_DIAGNOSTIC_ENTRY_H_
