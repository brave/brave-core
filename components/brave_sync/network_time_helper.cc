/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/network_time_helper.h"

#include <utility>

#include "base/sequence_checker.h"
#include "base/task/post_task.h"
#include "components/network_time/network_time_tracker.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

namespace brave_sync {

// static
NetworkTimeHelper* NetworkTimeHelper::GetInstance() {
  return base::Singleton<NetworkTimeHelper>::get();
}

NetworkTimeHelper::NetworkTimeHelper() {}
NetworkTimeHelper::~NetworkTimeHelper() {}

void NetworkTimeHelper::SetNetworkTimeTracker(
    network_time::NetworkTimeTracker* tracker) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  network_time_tracker_ = tracker;
}

void NetworkTimeHelper::GetNetworkTime(GetNetworkTimeCallback cb) {
  if (!network_time_for_test_.is_null()) {
    std::move(cb).Run(network_time_for_test_);
    return;
  }
  base::PostTask(FROM_HERE, {content::BrowserThread::UI},
                 base::BindOnce(&NetworkTimeHelper::GetNetworkTimeOnUIThread,
                                weak_ptr_factory_.GetWeakPtr(), std::move(cb)));
}

void NetworkTimeHelper::SetNetworkTimeForTest(const base::Time& time) {
  network_time_for_test_ = time;
}

void NetworkTimeHelper::GetNetworkTimeOnUIThread(GetNetworkTimeCallback cb) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::Time time;
  if (!network_time_tracker_ ||
      network_time_tracker_->GetNetworkTime(&time, nullptr) !=
          network_time::NetworkTimeTracker::NETWORK_TIME_AVAILABLE) {
    VLOG(1) << "Network time not available, using local time";
    time = base::Time::Now();
  }
  std::move(cb).Run(time);
}

}  // namespace brave_sync
