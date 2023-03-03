/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/covariate_manager.h"

#include <utility>

#include "base/check.h"
#include "base/containers/fixed_flat_map.h"
#include "base/time/time.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/covariates/log_entries/average_clickthrough_rate.h"
#include "bat/ads/internal/covariates/log_entries/last_notification_ad_was_clicked.h"
#include "bat/ads/internal/covariates/log_entries/notification_ad_event.h"
#include "bat/ads/internal/covariates/log_entries/notification_ad_served_at.h"
#include "bat/ads/internal/covariates/log_entries/number_of_user_activity_events.h"
#include "bat/ads/internal/covariates/log_entries/time_since_last_user_activity_event.h"
#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom.h"

namespace ads {

namespace {

CovariateManager* g_covariate_logs_instance = nullptr;

constexpr auto kUserActivityEventToCovariateTypesMapping =
    base::MakeFixedFlatMap<UserActivityEventType,
                           std::pair<brave_federated::mojom::CovariateType,
                                     brave_federated::mojom::CovariateType>>(
        {{UserActivityEventType::kBrowserDidBecomeActive,
          {brave_federated::mojom::CovariateType::
               kNumberOfBrowserDidBecomeActiveEvents,
           brave_federated::mojom::CovariateType::
               kTimeSinceLastBrowserDidBecomeActiveEvent}},
         {UserActivityEventType::kBrowserDidEnterForeground,
          {brave_federated::mojom::CovariateType::
               kNumberOfBrowserDidEnterForegroundEvents,
           brave_federated::mojom::CovariateType::
               kTimeSinceLastBrowserDidEnterForegroundEvent}},
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
           brave_federated::mojom::CovariateType::
               kTimeSinceLastClosedTabEvent}},
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
           brave_federated::mojom::CovariateType::
               kTimeSinceLastTypedUrlEvent}}});

constexpr base::TimeDelta kAverageClickthroughRateTimeWindows[] = {
    base::Days(1), base::Days(7), base::Days(28)};

}  // namespace

CovariateManager::CovariateManager() {
  DCHECK(!g_covariate_logs_instance);
  g_covariate_logs_instance = this;

  SetLogEntry(std::make_unique<LastNotificationAdWasClicked>());

  for (const auto& user_activity_event_to_covariate_types_mapping :
       kUserActivityEventToCovariateTypesMapping) {
    const UserActivityEventType event_type =
        user_activity_event_to_covariate_types_mapping.first;

    const brave_federated::mojom::CovariateType
        number_of_events_covariate_type =
            user_activity_event_to_covariate_types_mapping.second.first;
    SetLogEntry(std::make_unique<NumberOfUserActivityEvents>(
        event_type, number_of_events_covariate_type));

    const brave_federated::mojom::CovariateType
        time_since_last_event_covariate_type =
            user_activity_event_to_covariate_types_mapping.second.second;
    SetLogEntry(std::make_unique<TimeSinceLastUserActivityEvent>(
        event_type, time_since_last_event_covariate_type));
  }

  for (const auto& average_clickthrough_rate_time_window :
       kAverageClickthroughRateTimeWindows) {
    SetLogEntry(std::make_unique<AverageClickthroughRate>(
        average_clickthrough_rate_time_window));
  }
}

CovariateManager::~CovariateManager() {
  DCHECK_EQ(this, g_covariate_logs_instance);
  g_covariate_logs_instance = nullptr;
}

// static
CovariateManager* CovariateManager::GetInstance() {
  DCHECK(g_covariate_logs_instance);
  return g_covariate_logs_instance;
}

// static
bool CovariateManager::HasInstance() {
  return g_covariate_logs_instance != nullptr;
}

void CovariateManager::SetLogEntry(
    std::unique_ptr<CovariateLogEntryInterface> entry) {
  DCHECK(entry);
  const brave_federated::mojom::CovariateType key = entry->GetType();
  covariate_log_entries_[key] = std::move(entry);
}

std::vector<brave_federated::mojom::CovariateInfoPtr>
CovariateManager::GetTrainingInstance() const {
  std::vector<brave_federated::mojom::CovariateInfoPtr> training_instance;
  for (const auto& covariate_log_entry : covariate_log_entries_) {
    const CovariateLogEntryInterface* const entry =
        covariate_log_entry.second.get();
    DCHECK(entry);

    brave_federated::mojom::CovariateInfoPtr covariate =
        brave_federated::mojom::CovariateInfo::New();
    covariate->data_type = entry->GetDataType();
    covariate->type = entry->GetType();
    covariate->value = entry->GetValue();
    training_instance.push_back(std::move(covariate));
  }

  return training_instance;
}

void CovariateManager::SetNotificationAdServedAt(const base::Time time) {
  auto notification_ad_served_at = std::make_unique<NotificationAdServedAt>();
  notification_ad_served_at->SetTime(time);
  SetLogEntry(std::move(notification_ad_served_at));
}

void CovariateManager::SetNotificationAdEvent(
    const mojom::NotificationAdEventType event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  auto notification_ad_event = std::make_unique<NotificationAdEvent>();
  notification_ad_event->SetEventType(event_type);
  SetLogEntry(std::move(notification_ad_event));
}

void CovariateManager::LogTrainingInstance() const {
  std::vector<brave_federated::mojom::CovariateInfoPtr> training_instance =
      GetTrainingInstance();
  AdsClientHelper::GetInstance()->LogTrainingInstance(
      std::move(training_instance));
}

}  // namespace ads
