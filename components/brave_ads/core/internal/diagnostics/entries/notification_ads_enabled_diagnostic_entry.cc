/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/notification_ads_enabled_diagnostic_entry.h"

#include "brave/components/brave_ads/core/internal/common/strings/string_conversions_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"

namespace brave_ads {

namespace {
constexpr char kName[] = "Notification ads enabled";
}  // namespace

DiagnosticEntryType NotificationAdsEnabledDiagnosticEntry::GetType() const {
  return DiagnosticEntryType::kNotificationAdsEnabled;
}

std::string NotificationAdsEnabledDiagnosticEntry::GetName() const {
  return kName;
}

std::string NotificationAdsEnabledDiagnosticEntry::GetValue() const {
  return BoolToString(IsNotificationAdsEnabled());
}

}  // namespace brave_ads
