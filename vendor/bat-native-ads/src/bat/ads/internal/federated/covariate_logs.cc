/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/federated/covariate_logs.h"

#include <utility>

#include "bat/ads/internal/logging.h"

#include "base/check.h"
#include "base/time/time.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/federated/covariate_log_entry.h"
#include "bat/ads/internal/federated/log_entries/ad_notification_clicked_covariate_log_entry.h"
#include "bat/ads/internal/federated/log_entries/ad_notification_impression_served_at_covariate_log_entry.h"
#include "bat/ads/internal/federated/log_entries/ad_notification_locale_country_at_time_of_serving_covariate_log_entry.h"
#include "bat/ads/internal/federated/log_entries/ad_notification_number_of_tabs_opened_in_past_30_minutes_log_entry.h"

namespace ads {

CovariateLogs::CovariateLogs() {
  SetCovariateLogEntry(
      std::make_unique<AdNotificationNumberOfTabsOpenedInPast30Minutes>());
  SetCovariateLogEntry(
      std::make_unique<
          AdNotificationLocaleCountryAtTimeOfServingCovariateLogEntry>());
}

CovariateLogs::~CovariateLogs() = default;

void CovariateLogs::SetCovariateLogEntry(
    std::unique_ptr<CovariateLogEntry> entry) {
  DCHECK(entry);
  brave_federated::mojom::CovariateType key = entry->GetCovariateType();
  covariate_log_entries_[key] = std::move(entry);
}

brave_federated::mojom::TrainingCovariatesPtr
CovariateLogs::GetTrainingCovariates() const {
  brave_federated::mojom::TrainingCovariatesPtr training_covariates =
      brave_federated::mojom::TrainingCovariates::New();
  for (const auto& covariate_log_entry : covariate_log_entries_) {
    const CovariateLogEntry* entry = covariate_log_entry.second.get();
    DCHECK(entry);

    brave_federated::mojom::CovariatePtr covariate =
        brave_federated::mojom::Covariate::New();
    covariate->data_type = entry->GetDataType();
    covariate->covariate_type = entry->GetCovariateType();
    covariate->value = entry->GetValue();
    training_covariates->covariates.push_back(std::move(covariate));
  }

  return training_covariates;
}

void CovariateLogs::SetAdNotificationImpressionServedAt(
    const base::Time impression_served_at) {
  auto impression_served_at_covariate_log_entry =
      std::make_unique<AdNotificationImpressionServedAtCovariateLogEntry>();
  impression_served_at_covariate_log_entry->SetLastImpressionAt(
      base::Time::Now());
  SetCovariateLogEntry(std::move(impression_served_at_covariate_log_entry));
}

void CovariateLogs::SetAdNotificationWasClicked(bool was_clicked) {
  auto ad_notification_clicked_covariate_log_entry =
      std::make_unique<AdNotificationClickedCovariateLogEntry>();
  ad_notification_clicked_covariate_log_entry->SetClicked(was_clicked);
  SetCovariateLogEntry(std::move(ad_notification_clicked_covariate_log_entry));
}

void CovariateLogs::LogTrainingCovariates() {
  brave_federated::mojom::TrainingCovariatesPtr training_covariates =
      GetTrainingCovariates();
  AdsClientHelper::Get()->LogTrainingCovariates(std::move(training_covariates));
}

}  // namespace ads
