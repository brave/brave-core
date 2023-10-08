/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/predictors_manager.h"

#include <utility>

#include "base/check.h"
#include "base/containers/fixed_flat_map.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/average_clickthrough_rate_predictor_variable.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/last_notification_ad_was_clicked_predictor_variable.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/number_of_user_activity_events_predictor_variable.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/time_since_last_user_activity_event_predictor_variable.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom.h"

namespace brave_ads {

namespace {

constexpr auto kUserActivityEventToPredictorVariableTypeMapping =
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

PredictorsManager::PredictorsManager() {
  SetPredictorVariable(
      std::make_unique<LastNotificationAdWasClickedPredictorVariable>());

  for (const auto& [user_activity_event_type, predictor_variable_type] :
       kUserActivityEventToPredictorVariableTypeMapping) {
    const auto& [number_of_user_activity_events,
                 time_since_last_user_activity_event] = predictor_variable_type;

    SetPredictorVariable(
        std::make_unique<NumberOfUserActivityEventsPredictorVariable>(
            user_activity_event_type, number_of_user_activity_events));

    SetPredictorVariable(
        std::make_unique<TimeSinceLastUserActivityEventPredictorVariable>(
            user_activity_event_type, time_since_last_user_activity_event));
  }

  for (const auto& time_window : kAverageClickthroughRateTimeWindows) {
    SetPredictorVariable(
        std::make_unique<AverageClickthroughRatePredictorVariable>(
            time_window));
  }
}

PredictorsManager::~PredictorsManager() = default;

// static
PredictorsManager& PredictorsManager::GetInstance() {
  return GlobalState::GetInstance()->GetPredictorsManager();
}

void PredictorsManager::SetPredictorVariable(
    std::unique_ptr<PredictorVariableInterface> predictor_variable) {
  CHECK(predictor_variable);

  const brave_federated::mojom::CovariateType type =
      predictor_variable->GetType();
  predictor_variables_[type] = std::move(predictor_variable);
}

std::vector<brave_federated::mojom::CovariateInfoPtr>
PredictorsManager::GetTrainingSample() const {
  std::vector<brave_federated::mojom::CovariateInfoPtr> training_sample;

  for (const auto& [_, predictor_variable] : predictor_variables_) {
    CHECK(predictor_variable);

    brave_federated::mojom::CovariateInfoPtr predictor =
        brave_federated::mojom::CovariateInfo::New();
    predictor->data_type = predictor_variable->GetDataType();
    predictor->type = predictor_variable->GetType();
    predictor->value = predictor_variable->GetValue();

    training_sample.push_back(std::move(predictor));
  }

  return training_sample;
}

void PredictorsManager::AddTrainingSample() const {
  std::vector<brave_federated::mojom::CovariateInfoPtr> training_sample =
      GetTrainingSample();
  AddFederatedLearningPredictorTrainingSample(std::move(training_sample));
}

}  // namespace brave_ads
