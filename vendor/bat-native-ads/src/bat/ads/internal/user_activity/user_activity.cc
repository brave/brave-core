/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/user_activity.h"

#include "base/time/time.h"

namespace ads {

namespace {

UserActivity* g_user_activity = nullptr;

const size_t kMaximumUserActivityEventHistoryEntries = 100;

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
  if (history_.find(event_type) == history_.end()) {
    history_.insert({event_type, {}});
  }

  const int64_t timestamp = static_cast<int64_t>(base::Time::Now().ToDoubleT());
  history_.at(event_type).push_front(timestamp);

  if (history_.at(event_type).size() >
      kMaximumUserActivityEventHistoryEntries) {
    history_.at(event_type).pop_back();
  }
}

const UserActivityEventHistoryMap& UserActivity::get_history() const {
  return history_;
}

}  // namespace ads
