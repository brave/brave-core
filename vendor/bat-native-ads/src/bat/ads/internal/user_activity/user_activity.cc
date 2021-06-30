/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/user_activity.h"

#include <cstdint>
#include <string>

#include "base/strings/string_number_conversions.h"
#include "bat/ads/internal/features/user_activity/user_activity_features.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/user_activity/page_transition_util.h"
#include "bat/ads/internal/user_activity/user_activity_scoring.h"
#include "bat/ads/internal/user_activity/user_activity_util.h"

namespace ads {

namespace {

UserActivity* g_user_activity = nullptr;

void LogEvent(const UserActivityEventType event_type) {
  const UserActivityTriggers triggers =
      ToUserActivityTriggers(features::user_activity::GetTriggers());

  const base::TimeDelta time_window = features::user_activity::GetTimeWindow();
  const UserActivityEvents events =
      UserActivity::Get()->GetHistoryForTimeWindow(time_window);

  const double score = GetUserActivityScore(triggers, events);

  const double threshold = features::user_activity::GetThreshold();

  const std::string encoded_event_type =
      base::HexEncode(&event_type, sizeof(int8_t));

  BLOG(6, "Triggered event: "
              << encoded_event_type << " (" << score << ":" << threshold << ":"
              << features::user_activity::GetTimeWindow() << ")");
}

}  // namespace

UserActivity::UserActivity() {
  DCHECK_EQ(g_user_activity, nullptr);
  g_user_activity = this;
}

UserActivity::~UserActivity() {
  DCHECK(g_user_activity);
  g_user_activity = nullptr;
}

// static
UserActivity* UserActivity::Get() {
  DCHECK(g_user_activity);
  return g_user_activity;
}

// static
bool UserActivity::HasInstance() {
  return g_user_activity;
}

void UserActivity::RecordEvent(const UserActivityEventType event_type) {
  UserActivityEventInfo user_activity_event;
  user_activity_event.type = event_type;
  user_activity_event.time = base::Time::Now();

  history_.push_back(user_activity_event);

  if (history_.size() > kMaximumHistoryEntries) {
    history_.pop_front();
  }

  LogEvent(event_type);
}

void UserActivity::RecordEventForPageTransition(const PageTransitionType type) {
  if (IsNewNavigation(type)) {
    RecordEvent(UserActivityEventType::kNewNavigation);
  }

  if (DidUseBackOrFowardButtonToTriggerNavigation(type)) {
    RecordEvent(UserActivityEventType::kClickedBackOrForwardNavigationButtons);
  }

  if (DidUseAddressBarToTriggerNavigation(type)) {
    RecordEvent(UserActivityEventType::kUsedAddressBar);
  }

  if (DidNavigateToHomePage(type)) {
    RecordEvent(UserActivityEventType::kClickedHomePageButton);
  }

  if (DidTransitionFromExternalApplication(type)) {
    RecordEvent(UserActivityEventType::kOpenedLinkFromExternalApplication);
  }

  const absl::optional<UserActivityEventType> event_type =
      ToUserActivityEventType(type);
  if (!event_type) {
    return;
  }

  RecordEvent(event_type.value());
}

void UserActivity::RecordEventForPageTransitionFromInt(const int32_t type) {
  const PageTransitionType page_transition_type =
      static_cast<PageTransitionType>(type);

  RecordEventForPageTransition(page_transition_type);
}

UserActivityEvents UserActivity::GetHistoryForTimeWindow(
    const base::TimeDelta time_window) const {
  UserActivityEvents filtered_history = history_;

  const base::Time time = base::Time::Now() - time_window;

  const auto iter =
      std::remove_if(filtered_history.begin(), filtered_history.end(),
                     [&time](const UserActivityEventInfo& event) {
                       return event.time < time;
                     });

  filtered_history.erase(iter, filtered_history.end());

  return filtered_history;
}

}  // namespace ads
