/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_manager.h"

#include <cstddef>
#include <iterator>
#include <optional>

#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/application_state/browser_manager.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_info.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/page_transition_util.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_feature.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_scoring.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_trigger_info.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_util.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads {

namespace {

void LogEvent(const UserActivityEventType event_type) {
  const UserActivityTriggerList triggers =
      ToUserActivityTriggers(kUserActivityTriggers.Get());

  const UserActivityEventList events =
      UserActivityManager::GetInstance().GetHistoryForTimeWindow(
          kUserActivityTimeWindow.Get());

  BLOG(6, "Triggered event: " << base::HexEncode(&event_type, sizeof(int8_t))
                              << " (" << GetUserActivityScore(triggers, events)
                              << ":" << kUserActivityThreshold.Get() << ":"
                              << kUserActivityTimeWindow.Get() << ")");
}

}  // namespace

UserActivityManager::UserActivityManager() {
  GetAdsClient()->AddObserver(this);
  BrowserManager::GetInstance().AddObserver(this);
  TabManager::GetInstance().AddObserver(this);
}

UserActivityManager::~UserActivityManager() {
  GetAdsClient()->RemoveObserver(this);
  BrowserManager::GetInstance().RemoveObserver(this);
  TabManager::GetInstance().RemoveObserver(this);
}

// static
UserActivityManager& UserActivityManager::GetInstance() {
  return GlobalState::GetInstance()->GetUserActivityManager();
}

void UserActivityManager::RecordEvent(const UserActivityEventType event_type) {
  if (!UserHasJoinedBraveRewards()) {
    // User has not joined Brave Rewards, so we don't need to track user
    // activity.
    return;
  }

  user_activity_events_.push_back(UserActivityEventInfo{
      .type = event_type, .created_at = base::Time::Now()});

  if (user_activity_events_.size() >
      static_cast<size_t>(kMaximumUserActivityEvents.Get())) {
    user_activity_events_.pop_front();
  }

  LogEvent(event_type);
}

UserActivityEventList UserActivityManager::GetHistoryForTimeWindow(
    const base::TimeDelta time_window) const {
  UserActivityEventList filtered_history;

  const base::Time time = base::Time::Now() - time_window;

  std::copy_if(user_activity_events_.cbegin(), user_activity_events_.cend(),
               std::back_inserter(filtered_history),
               [time](const UserActivityEventInfo& event) {
                 return event.created_at >= time;
               });

  return filtered_history;
}

///////////////////////////////////////////////////////////////////////////////

void UserActivityManager::RecordEventForPageTransition(
    const PageTransitionType type) {
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

  const std::optional<UserActivityEventType> event_type =
      ToUserActivityEventType(type);
  if (!event_type) {
    return;
  }

  RecordEvent(*event_type);
}

void UserActivityManager::OnNotifyDidInitializeAds() {
  RecordEvent(UserActivityEventType::kInitializedAds);
}

void UserActivityManager::OnNotifyUserGestureEventTriggered(
    const int32_t type) {
  const auto page_transition_type = static_cast<PageTransitionType>(type);

  RecordEventForPageTransition(page_transition_type);
}

void UserActivityManager::OnBrowserDidBecomeActive() {
  RecordEvent(UserActivityEventType::kBrowserDidBecomeActive);
}

void UserActivityManager::OnBrowserDidResignActive() {
  RecordEvent(UserActivityEventType::kBrowserDidResignActive);
}

void UserActivityManager::OnBrowserDidEnterForeground() {
  RecordEvent(UserActivityEventType::kBrowserDidEnterForeground);
}

void UserActivityManager::OnBrowserDidEnterBackground() {
  RecordEvent(UserActivityEventType::kBrowserDidEnterBackground);
}

void UserActivityManager::OnDidOpenNewTab(const TabInfo& /*tab*/) {
  RecordEvent(UserActivityEventType::kOpenedNewTab);
}

void UserActivityManager::OnTabDidChangeFocus(const int32_t /*tab_id*/) {
  RecordEvent(UserActivityEventType::kTabChangedFocus);
}

void UserActivityManager::OnTabDidChange(const TabInfo& /*tab*/) {
  RecordEvent(UserActivityEventType::kTabDidChange);
}

void UserActivityManager::OnDidCloseTab(const int32_t /*tab_id*/) {
  RecordEvent(UserActivityEventType::kClosedTab);
}

void UserActivityManager::OnTabDidStartPlayingMedia(const int32_t /*tab_id*/) {
  RecordEvent(UserActivityEventType::kTabStartedPlayingMedia);
}

void UserActivityManager::OnTabDidStopPlayingMedia(const int32_t /*tab_id*/) {
  RecordEvent(UserActivityEventType::kTabStoppedPlayingMedia);
}

}  // namespace brave_ads
