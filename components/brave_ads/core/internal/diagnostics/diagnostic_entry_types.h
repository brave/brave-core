/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_ENTRY_TYPES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_ENTRY_TYPES_H_

namespace brave_ads {

// Diagnostic entries should be visually sorted based on the enum order.
enum class DiagnosticEntryType {
  kDeviceId,
  kOptedInToBraveNewsAds,
  kOptedInToNewTabPageAds,
  kOptedInToNotificationAds,
  kOptedInToSearchResultAds,
  kLocale,
  kCatalogId,
  kCatalogLastUpdated,
  kLastUnIdleTime
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_ENTRY_TYPES_H_
