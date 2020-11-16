/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/user_activity.h"

#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/time_util.h"

namespace ads {

namespace {
const size_t kMaximumUserActivityEntries = 100;
}

UserActivity::UserActivity() = default;

UserActivity::~UserActivity() = default;

void UserActivity::RecordActivityForType(
    const UserActivityType type) {
  if (history_.find(type) == history_.end()) {
    history_.insert({type, {}});
  }

  const uint64_t timestamp =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());
  history_.at(type).push_front(timestamp);

  if (history_.at(type).size() > kMaximumUserActivityEntries) {
    history_.at(type).pop_back();
  }
}

const UserActivityHistoryMap& UserActivity::get_history() const {
  return history_;
}

///////////////////////////////////////////////////////////////////////////////

}  // namespace ads
