/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_DIAGNOSTIC_ENTRY_INTERFACE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_DIAGNOSTIC_ENTRY_INTERFACE_H_

#include <string>

#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_entry_types.h"

namespace brave_ads {

class DiagnosticEntryInterface {
 public:
  virtual ~DiagnosticEntryInterface() = default;

  // Returns the type of the diagnostic entry, which categorizes the entry for
  // diagnostic purposes.
  virtual DiagnosticEntryType GetType() const = 0;

  // Returns the name of the diagnostic entry, providing a human-readable
  // identifier for this entry.
  virtual std::string GetName() const = 0;

  // Returns the value associated with this diagnostic entry, representing
  // relevant diagnostic data.
  virtual std::string GetValue() const = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_DIAGNOSTIC_ENTRY_INTERFACE_H_
