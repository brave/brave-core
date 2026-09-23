/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define BRAVE_EXPIRE_HISTORY_BACKEND_START_EXPIRING_OLD_STUFF \
  work_queue_ = {};                                           \
  weak_factory_.InvalidateWeakPtrs();                         \
  if (expiration_threshold_.is_max()) {                       \
    return;                                                   \
  }

#include <components/history/core/browser/expire_history_backend.cc>

namespace history {

void ExpireHistoryBackend::UpdateExpirationThreshold(
    base::TimeDelta threshold) {
  // StartExpiringOldStuff() schedules a self-perpetuating cleanup loop, so pref
  // changes must update the running loop instead of starting another one. If
  // the new value is "forever", cancel pending expiration work. If the previous
  // value was "forever", the loop is no longer running and must be started
  // once. Otherwise the existing loop will pick up the new threshold on its
  // next pass.
  if (threshold.is_max()) {
    expiration_threshold_ = threshold;
    work_queue_ = {};
    weak_factory_.InvalidateWeakPtrs();
    return;
  }

  if (expiration_threshold_.is_max()) {
    StartExpiringOldStuff(threshold);
    return;
  }

  expiration_threshold_ = threshold;
}

}  // namespace history

#undef BRAVE_EXPIRE_HISTORY_BACKEND_START_EXPIRING_OLD_STUFF
