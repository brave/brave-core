/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/covariate_logs.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/no_destructor.h"
#include "base/time/time.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/covariates/log_entries/ad_notification_clicked.h"
#include "bat/ads/internal/covariates/log_entries/ad_notification_served_at.h"
#include "bat/ads/internal/covariates/log_entries/average_clickthrough_rate.h"
#include "bat/ads/internal/covariates/log_entries/last_ad_notification_was_clicked.h"
#include "bat/ads/internal/covariates/log_entries/number_of_user_activity_events.h"
#include "bat/ads/internal/covariates/log_entries/time_since_last_user_activity_event.h"

namespace ads {

namespace {

CovariateLogs* g_covariate_logs_instance = nullptr;

using UserActivityEventToCovariateTypesMapping =
    base::flat_map<UserActivityEventType,
                   std::pair<brave_federated::mojom::CovariateType,
                             brave_federated::mojom::CovariateType>>;
UserActivityEventToCovariateTypesMapping&
GetUserActivityEventToCovariateTypesMapping() {
  static base::NoDestructor<UserActivityEventToCovariateTypesMapping> mappings(
      {{UserActivityEventType::kBrowserDidBecomeActive,
        {brave_federated::mojom::CovariateType::
             kNumberOfBrowserDidBecomeActiveEvents,
         brave_federated::mojom::CovariateType::
             kTimeSinceLastBrowserDidBecomeActiveEvent}},
       {UserActivityEventType::kBrowserDidBecomeActive,
        {brave_federated::mojom::CovariateType::
             kNumberOfBrowserWindowIsActiveEvents,
         brave_federated::mojom::CovariateType::
             kTimeSinceLastBrowserWindowIsActiveEvent}},
       {UserActivityEventType::kBrowserDidResignActive,
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
         brave_federated::mojom::CovariateType::kTimeSinceLastClosedTabEvent}},
       {UserActivityEventType::kTabChangedFocus,
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
       {UserActivityEventType::kTabStartedPlayingMedia,
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
         brave_federated::mojom::CovariateType::kTimeSinceLastTypedUrlEvent}}});
  return *mappings;
}

using AverageClickthroughRateTimeWindows = std::vector<base::TimeDelta>;
AverageClickthroughRateTimeWindows& GetAverageClickthroughRateTimeWindows() {
  static base::NoDestructor<std::vector<base::TimeDelta>> time_windows(
      {base::Days(1), base::Days(7), base::Days(28)});
  return *time_windows;
}

}  // namespace

// TODO(https://github.com/brave/brave-browser/issues/22310): Refactor
// CovariateLogs to Covariates
CovariateLogs::CovariateLogs() {
  DCHECK(!g_covariate_logs_instance);
  g_covariate_logs_instance = this;

  SetCovariateLogEntry(std::make_unique<LastAdNotificationWasClicked>());

  for (const auto& user_activity_event_to_covariate_types_mapping :
       GetUserActivityEventToCovariateTypesMapping()) {
    const UserActivityEventType event_type =
        user_activity_event_to_covariate_types_mapping.first;

    const brave_federated::mojom::CovariateType
        number_of_events_covariate_type =
            user_activity_event_to_covariate_types_mapping.second.first;
    SetCovariateLogEntry(std::make_unique<NumberOfUserActivityEvents>(
        event_type, number_of_events_covariate_type));

    const brave_federated::mojom::CovariateType
        time_since_last_event_covariate_type =
            user_activity_event_to_covariate_types_mapping.second.second;
    SetCovariateLogEntry(std::make_unique<TimeSinceLastUserActivityEvent>(
        event_type, time_since_last_event_covariate_type));
  }

  for (const auto& average_clickthrough_rate_time_window :
       GetAverageClickthroughRateTimeWindows()) {
    SetCovariateLogEntry(std::make_unique<AverageClickthroughRate>(
        average_clickthrough_rate_time_window));
  }
}

CovariateLogs::~CovariateLogs() {
  DCHECK_EQ(this, g_covariate_logs_instance);
  g_covariate_logs_instance = nullptr;
}

// static
CovariateLogs* CovariateLogs::Get() {
  DCHECK(g_covariate_logs_instance);
  return g_covariate_logs_instance;
}

// static
bool CovariateLogs::HasInstance() {
  return !!g_covariate_logs_instance;
}

void CovariateLogs::SetCovariateLogEntry(
    std::unique_ptr<CovariateLogEntryInterface> entry) {
  DCHECK(entry);
  brave_federated::mojom::CovariateType key = entry->GetCovariateType();
  covariate_log_entries_[key] = std::move(entry);
}

brave_federated::mojom::TrainingInstancePtr CovariateLogs::GetTrainingInstance()
    const {
  brave_federated::mojom::TrainingInstancePtr training_instance =
      brave_federated::mojom::TrainingInstance::New();
  for (const auto& covariate_log_entry : covariate_log_entries_) {
    const CovariateLogEntryInterface* entry = covariate_log_entry.second.get();
    DCHECK(entry);

    brave_federated::mojom::CovariatePtr covariate =
        brave_federated::mojom::Covariate::New();
    covariate->data_type = entry->GetDataType();
    covariate->covariate_type = entry->GetCovariateType();
    covariate->value = entry->GetValue();
    training_instance->covariates.push_back(std::move(covariate));
  }

  return training_instance;
}

void CovariateLogs::SetAdNotificationServedAt(const base::Time time) {
  auto ad_notification_served_at = std::make_unique<AdNotificationServedAt>();
  ad_notification_served_at->SetTime(time);
  SetCovariateLogEntry(std::move(ad_notification_served_at));
}

void CovariateLogs::SetAdNotificationClicked(bool clicked) {
  auto ad_notification_clicked = std::make_unique<AdNotificationClicked>();
  ad_notification_clicked->SetClicked(clicked);
  SetCovariateLogEntry(std::move(ad_notification_clicked));
}

void CovariateLogs::LogTrainingInstance() {
  brave_federated::mojom::TrainingInstancePtr training_instance =
      GetTrainingInstance();
  AdsClientHelper::Get()->LogTrainingInstance(std::move(training_instance));
}

}  // namespace ads
