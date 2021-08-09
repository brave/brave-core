/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DIAGNOSTICS_CATALOG_LAST_UPDATED_AD_DIAGNOSTICS_ENTRY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DIAGNOSTICS_CATALOG_LAST_UPDATED_AD_DIAGNOSTICS_ENTRY_H_

#include <string>

#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_entry.h"

namespace ads {

class CatalogLastUpdatedAdDiagnosticsEntry : public AdDiagnosticsEntry {
 public:
  CatalogLastUpdatedAdDiagnosticsEntry();
  CatalogLastUpdatedAdDiagnosticsEntry(
      const CatalogLastUpdatedAdDiagnosticsEntry&) = delete;
  CatalogLastUpdatedAdDiagnosticsEntry& operator=(
      const CatalogLastUpdatedAdDiagnosticsEntry&) = delete;
  ~CatalogLastUpdatedAdDiagnosticsEntry() override;

  // AdDiagnosticsEntry
  AdDiagnosticsEntryType GetEntryType() const override;
  std::string GetKey() const override;
  std::string GetValue() const override;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DIAGNOSTICS_CATALOG_LAST_UPDATED_AD_DIAGNOSTICS_ENTRY_H_
