/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/browsing/user_activity.h"

#include <algorithm>
#include <string>

#include "base/check_op.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/browser_manager/browser_manager.h"
#include "bat/ads/internal/tab_manager/tab_manager.h"
#include "bat/ads/internal/user_activity/browsing/page_transition_util.h"
#include "bat/ads/internal/user_activity/browsing/user_activity_constants.h"
#include "bat/ads/internal/user_activity/browsing/user_activity_features.h"
#include "bat/ads/internal/user_activity/browsing/user_activity_scoring.h"
#include "bat/ads/internal/user_activity/browsing/user_activity_trigger_info_aliases.h"
#include "bat/ads/internal/user_activity/browsing/user_activity_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

namespace {

UserActivity* g_user_activity_instance = nullptr;

void LogEvent(const UserActivityEventType event_type) {
  const UserActivityTriggerList triggers =
      ToUserActivityTriggers(features::user_activity::GetTriggers());

  const base::TimeDelta time_window = features::user_activity::GetTimeWindow();
  const UserActivityEventList events =
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
  DCHECK(!g_user_activity_instance);
  g_user_activity_instance = this;

  BrowserManager::Get()->AddObserver(this);
  TabManager::Get()->AddObserver(this);
}

UserActivity::~UserActivity() {
  BrowserManager::Get()->RemoveObserver(this);
  TabManager::Get()->RemoveObserver(this);

  DCHECK_EQ(this, g_user_activity_instance);
  g_user_activity_instance = nullptr;
}

// static
UserActivity* UserActivity::Get() {
  DCHECK(g_user_activity_instance);
  return g_user_activity_instance;
}

// static
bool UserActivity::HasInstance() {
  return !!g_user_activity_instance;
}

void UserActivity::RecordEvent(const UserActivityEventType event_type) {
  UserActivityEventInfo user_activity_event;
  user_activity_event.type = event_type;
  user_activity_event.created_at = base::Time::Now();

  history_.push_back(user_activity_event);

  if (history_.size() > kMaximumHistoryItems) {
    history_.pop_front();
  }

  LogEvent(event_type);
}

void UserActivity::RecordEventForPageTransition(const int32_t type) {
  const PageTransitionType page_transition_type =
      static_cast<PageTransitionType>(type);

  RecordEventForPageTransition(page_transition_type);
}

UserActivityEventList UserActivity::GetHistoryForTimeWindow(
    const base::TimeDelta time_window) const {
  UserActivityEventList filtered_history = history_;

  const base::Time time = base::Time::Now() - time_window;

  const auto iter =
      std::remove_if(filtered_history.begin(), filtered_history.end(),
                     [&time](const UserActivityEventInfo& event) {
                       return event.created_at < time;
                     });

  filtered_history.erase(iter, filtered_history.end());

  return filtered_history;
}

///////////////////////////////////////////////////////////////////////////////

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

void UserActivity::OnBrowserDidBecomeActive() {
  RecordEvent(UserActivityEventType::kBrowserDidBecomeActive);
}

void UserActivity::OnBrowserDidResignActive() {
  RecordEvent(UserActivityEventType::kBrowserDidResignActive);
}

void UserActivity::OnBrowserDidEnterForeground() {
  RecordEvent(UserActivityEventType::kBrowserDidEnterForeground);
}

void UserActivity::OnBrowserDidEnterBackground() {
  RecordEvent(UserActivityEventType::kBrowserDidEnterBackground);
}

void UserActivity::OnTabDidChangeFocus(const int32_t id) {
  RecordEvent(UserActivityEventType::kTabChangedFocus);
}

void UserActivity::OnTabDidChange(const int32_t id) {
  RecordEvent(UserActivityEventType::kTabUpdated);
}

void UserActivity::OnDidOpenNewTab(const int32_t id) {
  RecordEvent(UserActivityEventType::kOpenedNewTab);
}

void UserActivity::OnDidCloseTab(const int32_t id) {
  RecordEvent(UserActivityEventType::kClosedTab);
}

void UserActivity::OnTabDidStartPlayingMedia(const int32_t id) {
  RecordEvent(UserActivityEventType::kTabStartedPlayingMedia);
}

void UserActivity::OnTabDidStopPlayingMedia(const int32_t id) {
  RecordEvent(UserActivityEventType::kTabStoppedPlayingMedia);
}

}  // namespace ads
