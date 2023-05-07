/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/reminder_util.h"

#include "base/guid.h"
#include "base/notreached.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/notification_ad_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"

namespace brave_ads {

namespace {

base::Value::Dict GetClickedSameAdMultipleTimesReminder() {
  base::Value::Dict dict;

  dict.Set(kNotificationAdPlacementIdKey,
           base::GUID::GenerateRandomV4().AsLowercaseString());
  dict.Set(
      kNotificationAdTitleKey,
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_BRAVE_ADS_NOTIFICATION_CLICKED_SAME_AD_MULTIPLE_TIMES_TITLE));
  dict.Set(kNotificationAdBodyKey,
           brave_l10n::GetLocalizedResourceUTF16String(
               IDS_BRAVE_ADS_NOTIFICATION_CLICKED_SAME_AD_MULTIPLE_TIMES_BODY));
  dict.Set(kNotificationAdTargetUrlKey,
           "https://support.brave.com/hc/en-us/articles/14648356808845");

  return dict;
}

base::Value::Dict GetExternalWalletConnectedReminder() {
  base::Value::Dict dict;

  dict.Set(kNotificationAdPlacementIdKey,
           base::GUID::GenerateRandomV4().AsLowercaseString());
  dict.Set(kNotificationAdTitleKey,
           brave_l10n::GetLocalizedResourceUTF16String(
               IDS_BRAVE_ADS_NOTIFICATION_EXTERNAL_WALLET_CONNECTED_TITLE));
  dict.Set(kNotificationAdBodyKey,
           brave_l10n::GetLocalizedResourceUTF16String(
               IDS_BRAVE_ADS_NOTIFICATION_EXTERNAL_WALLET_CONNECTED_BODY));
  dict.Set(kNotificationAdTargetUrlKey,
           "https://support.brave.com/hc/en-us/articles/14648356808845");

  return dict;
}

}  // namespace

absl::optional<base::Value::Dict> GetReminder(const mojom::ReminderType type) {
  switch (type) {
    case mojom::ReminderType::kClickedSameAdMultipleTimes: {
      return GetClickedSameAdMultipleTimesReminder();
    }

    case mojom::ReminderType::kExternalWalletConnected: {
      return GetExternalWalletConnectedReminder();
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for mojom::ReminderType: " << type;
}

}  // namespace brave_ads
