/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/federated/log_entries/ad_notification_locale_country_at_time_of_serving_covariate_log_entry.h"

#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"

namespace ads {

AdNotificationLocaleCountryAtTimeOfServingCovariateLogEntry::
    AdNotificationLocaleCountryAtTimeOfServingCovariateLogEntry() = default;

AdNotificationLocaleCountryAtTimeOfServingCovariateLogEntry::
    ~AdNotificationLocaleCountryAtTimeOfServingCovariateLogEntry() = default;

brave_federated::mojom::DataType
AdNotificationLocaleCountryAtTimeOfServingCovariateLogEntry::GetDataType()
    const {
  return brave_federated::mojom::DataType::kString;
}

brave_federated::mojom::CovariateType
AdNotificationLocaleCountryAtTimeOfServingCovariateLogEntry::GetCovariateType()
    const {
  return brave_federated::mojom::CovariateType::
      kAdNotificationLocaleCountryAtTimeOfServing;
}

std::string
AdNotificationLocaleCountryAtTimeOfServingCovariateLogEntry::GetValue() const {
  const std::string locale =
      brave_l10n::LocaleHelper::GetInstance()->GetLocale();
  return brave_l10n::GetCountryCode(locale);
}

}  // namespace ads
