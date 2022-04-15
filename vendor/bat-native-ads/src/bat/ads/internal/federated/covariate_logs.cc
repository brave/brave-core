/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/federated/covariate_logs.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/flat_map.h"
#include "base/no_destructor.h"
#include "base/time/time.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/federated/covariate_log_entry.h"
#include "bat/ads/internal/federated/log_entries/ad_notification_clicked_covariate_log_entry.h"
#include "bat/ads/internal/federated/log_entries/ad_notification_impression_served_at_covariate_log_entry.h"
#include "bat/ads/internal/federated/log_entries/ad_notification_locale_country_at_time_of_serving_covariate_log_entry.h"
#include "bat/ads/internal/federated/log_entries/ad_notification_number_of_tabs_opened_in_past_30_minutes_log_entry.h"
#include "bat/ads/internal/federated/log_entries/average_clickthrough_rate.h"
#include "bat/ads/internal/federated/log_entries/last_ad_notification_was_clicked.h"
#include "bat/ads/internal/federated/log_entries/number_of_user_activity_events.h"
#include "bat/ads/internal/federated/log_entries/time_since_last_user_activity_event.h"
#include "bat/ads/internal/logging.h"

namespace ads {

namespace {

CovariateLogs* g_covariate_logs = nullptr;

static base::NoDestructor<
    base::flat_map<UserActivityEventType,
                   std::pair<brave_federated::mojom::CovariateType,
                             brave_federated::mojom::CovariateType>>>
    kUserActivityEventMappings(
        {{UserActivityEventType::kBrowserDidBecomeActive,
          {brave_federated::mojom::CovariateType::
               kNumberOfBrowserDidBecomeActiveEvents,
           brave_federated::mojom::CovariateType::
               kTimeSinceLastBrowserDidBecomeActiveEvent}},
         {UserActivityEventType::kBrowserWindowIsActive,
          {brave_federated::mojom::CovariateType::
               kNumberOfBrowserWindowIsActiveEvents,
           brave_federated::mojom::CovariateType::
               kTimeSinceLastBrowserWindowIsActiveEvent}},
         {UserActivityEventType::kBrowserWindowIsInactive,
          {brave_federated::mojom::CovariateType::
               kNumberOfBrowserWindowIsInactiveEvents,
           brave_federated::mojom::CovariateType::
               kTimeSinceLastBrowserWindowIsInactiveEvent}},
         {UserActivityEventType::kClickedBackOrForwardNavigationButtons,
          {brave_federated::mojom::CovariateType::
               kNumberOfClickedBackOrForwardNavigationButtonsEvents,
           brave_federated::mojom::CovariateType::
               kTimeSinceLastClickedBackOrForwardNavigationButtonsEvent}},
         {UserActivityEventType::kClickedLink,
          {brave_federated::mojom::CovariateType::kNumberOfClickedLinkEvents,
           brave_federated::mojom::CovariateType::
               kTimeSinceLastClickedLinkEvent}},
         {UserActivityEventType::kClickedReloadButton,
          {brave_federated::mojom::CovariateType::
               kNumberOfClickedReloadButtonEvents,
           brave_federated::mojom::CovariateType::
               kTimeSinceLastClickedReloadButtonEvent}},
         {UserActivityEventType::kClosedTab,
          {brave_federated::mojom::CovariateType::kNumberOfClosedTabEvents,
           brave_federated::mojom::CovariateType::
               kTimeSinceLastClosedTabEvent}},
         {UserActivityEventType::kFocusedOnExistingTab,
          {brave_federated::mojom::CovariateType::
               kNumberOfFocusedOnExistingTabEvents,
           brave_federated::mojom::CovariateType::
               kTimeSinceLastFocusedOnExistingTabEvent}},
         {UserActivityEventType::kNewNavigation,
          {brave_federated::mojom::CovariateType::kNumberOfNewNavigationEvents,
           brave_federated::mojom::CovariateType::
               kTimeSinceLastNewNavigationEvent}},
         {UserActivityEventType::kOpenedNewTab,
          {brave_federated::mojom::CovariateType::kNumberOfOpenedNewTabEvents,
           brave_federated::mojom::CovariateType::
               kTimeSinceLastOpenedNewTabEvent}},
         {UserActivityEventType::kPlayedMedia,
          {brave_federated::mojom::CovariateType::kNumberOfPlayedMediaEvents,
           brave_federated::mojom::CovariateType::
               kTimeSinceLastPlayedMediaEvent}},
         {UserActivityEventType::kSubmittedForm,
          {brave_federated::mojom::CovariateType::kNumberOfSubmittedFormEvents,
           brave_federated::mojom::CovariateType::
               kTimeSinceLastSubmittedFormEvent}},
         {UserActivityEventType::kTypedAndSelectedNonUrl,
          {brave_federated::mojom::CovariateType::
               kNumberOfTypedAndSelectedNonUrlEvents,
           brave_federated::mojom::CovariateType::
               kTimeSinceLastTypedAndSelectedNonUrlEvent}},
         {UserActivityEventType::kTypedKeywordOtherThanDefaultSearchProvider,
          {brave_federated::mojom::CovariateType::
               kNumberOfTypedKeywordOtherThanDefaultSearchProviderEvents,
           brave_federated::mojom::CovariateType::
               kTimeSinceLastTypedKeywordOtherThanDefaultSearchProviderEvent}},
         {UserActivityEventType::kTypedUrl,
          {brave_federated::mojom::CovariateType::kNumberOfTypedUrlEvents,
           brave_federated::mojom::CovariateType::
               kTimeSinceLastTypedUrlEvent}}});

static base::NoDestructor<std::vector<base::TimeDelta>>
    kAverageClickthroughRateTimeWindows({base::Days(1), base::Days(7),
                                         base::Days(28)});

}  // namespace

// TODO(https://github.com/brave/brave-browser/issues/22310): Refactor
// CovariateLogs to Covariates
CovariateLogs::CovariateLogs() {
  DCHECK_EQ(g_covariate_logs, nullptr);
  g_covariate_logs = this;

  SetCovariateLogEntry(
      std::make_unique<
          AdNotificationLocaleCountryAtTimeOfServingCovariateLogEntry>());
  SetCovariateLogEntry(
      std::make_unique<AdNotificationNumberOfTabsOpenedInPast30Minutes>());
  SetCovariateLogEntry(std::make_unique<LastAdNotificationWasClicked>());

  for (const auto& average_clickthrough_rate_time_window :
       *kAverageClickthroughRateTimeWindows) {
    SetCovariateLogEntry(std::make_unique<AverageClickthroughRate>(
        average_clickthrough_rate_time_window));
  }

  for (const auto& user_activity_event_mapping : *kUserActivityEventMappings) {
    const UserActivityEventType event_type = user_activity_event_mapping.first;

    const brave_federated::mojom::CovariateType
        number_of_events_covariate_type =
            user_activity_event_mapping.second.first;
    SetCovariateLogEntry(std::make_unique<NumberOfUserActivityEvents>(
        event_type, number_of_events_covariate_type));

    const brave_federated::mojom::CovariateType
        time_since_last_event_covariate_type =
            user_activity_event_mapping.second.second;
    SetCovariateLogEntry(std::make_unique<TimeSinceLastUserActivityEvent>(
        event_type, time_since_last_event_covariate_type));
  }
}

CovariateLogs::~CovariateLogs() {
  DCHECK(g_covariate_logs);
  g_covariate_logs = nullptr;
}

// static
CovariateLogs* CovariateLogs::Get() {
  DCHECK(g_covariate_logs);
  return g_covariate_logs;
}

// static
bool CovariateLogs::HasInstance() {
  return g_covariate_logs;
}

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
      impression_served_at);
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
