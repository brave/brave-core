/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/reminder/reminder_util.h"

#include <string_view>

#include "base/check.h"
#include "base/notreached.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

constexpr std::string_view kReminderNotificationAdPlacementId =
    "e64373ac-2ca5-4f6b-b497-1f1d7ccd40c8";
constexpr std::string_view kReminderNotificationAdTargetUrl =
    "https://support.brave.app/hc/en-us/articles/14648356808845";

mojom::NotificationAdInfoPtr BuildClickedSameAdMultipleTimesReminder() {
  auto notification_ad = mojom::NotificationAdInfo::New();
  notification_ad->placement_id = kReminderNotificationAdPlacementId;
  notification_ad->title = base::UTF16ToUTF8(l10n_util::GetStringUTF16(
      IDS_BRAVE_ADS_NOTIFICATION_CLICKED_SAME_AD_MULTIPLE_TIMES_TITLE));
  notification_ad->body = base::UTF16ToUTF8(l10n_util::GetStringUTF16(
      IDS_BRAVE_ADS_NOTIFICATION_CLICKED_SAME_AD_MULTIPLE_TIMES_BODY));
  notification_ad->target_url = GURL(kReminderNotificationAdTargetUrl);
  return notification_ad;
}

mojom::NotificationAdInfoPtr BuildExternalWalletConnectedReminder() {
  auto notification_ad = mojom::NotificationAdInfo::New();
  notification_ad->placement_id = kReminderNotificationAdPlacementId;
  notification_ad->title = base::UTF16ToUTF8(l10n_util::GetStringUTF16(
      IDS_BRAVE_ADS_NOTIFICATION_EXTERNAL_WALLET_CONNECTED_TITLE));
  notification_ad->body = base::UTF16ToUTF8(l10n_util::GetStringUTF16(
      IDS_BRAVE_ADS_NOTIFICATION_EXTERNAL_WALLET_CONNECTED_BODY));
  notification_ad->target_url = GURL(kReminderNotificationAdTargetUrl);
  return notification_ad;
}

}  // namespace

mojom::NotificationAdInfoPtr BuildReminder(
    mojom::ReminderType mojom_reminder_type) {
  switch (mojom_reminder_type) {
    case mojom::ReminderType::kClickedSameAdMultipleTimes: {
      return BuildClickedSameAdMultipleTimesReminder();
    }

    case mojom::ReminderType::kExternalWalletConnected: {
      return BuildExternalWalletConnectedReminder();
    }
  }

  NOTREACHED() << "Unexpected value for mojom::ReminderType: "
               << mojom_reminder_type;
}

bool IsReminder(const std::string& placement_id) {
  return placement_id == kReminderNotificationAdPlacementId;
}

GURL GetReminderTargetUrl() {
  return GURL(kReminderNotificationAdTargetUrl);
}

}  // namespace brave_ads
