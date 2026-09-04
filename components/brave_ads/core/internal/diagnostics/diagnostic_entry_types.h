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
  kOptedInToNewTabPageAds,
  kNotificationAdsEnabled,
  kOptedInToSearchResultAds,
  kLanguage,
  kCountry,
  kSchemaVersion,
  kLastDatabaseMigrationFailureReason,
  kWalletValid,
  kWalletConnected,
  kIssuersValid,
  kCatalogId,
  kCatalogVersion,
  kCatalogLastUpdated,
  kLastUnIdleTime,
  kNewTabPageAdsSchemaVersion,
  kCatalogNextUpdate,
  kCatalogPermission,
  kNetworkConnectionPermission,
  kBrowserIsActivePermission,
  kFullScreenModePermission,
  kMediaPermission,
  kDoNotDisturbPermission,
  kIssuersPermission,
  kConfirmationTokensPermission,
  kUserActivityPermission,
  kCommandLinePermission,
  kCanShowNotificationsPermission,
  kTextClassificationResource,
  kPurchaseIntentResource,
  kAntiTargetingResource
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_ENTRY_TYPES_H_
