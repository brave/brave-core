/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_NETWORK_TIME_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_NETWORK_TIME_HELPER_H_

#include "base/callback.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"

namespace network_time {
class NetworkTimeTracker;
}  // namespace network_time

namespace brave_sync {

class NetworkTimeHelper {
 public:
  using GetNetworkTimeCallback = base::OnceCallback<void(const base::Time&)>;

  static NetworkTimeHelper* GetInstance();

  NetworkTimeHelper();
  NetworkTimeHelper(const NetworkTimeHelper&) = delete;
  NetworkTimeHelper& operator=(const NetworkTimeHelper&) = delete;
  virtual ~NetworkTimeHelper();

  void SetNetworkTimeTracker(network_time::NetworkTimeTracker* tracker);

  void GetNetworkTime(GetNetworkTimeCallback cb);

  void SetNetworkTimeForTest(const base::Time& time);

 private:
  friend struct base::DefaultSingletonTraits<NetworkTimeHelper>;

  void GetNetworkTimeOnUIThread(GetNetworkTimeCallback cb);

  base::Time network_time_for_test_;

  SEQUENCE_CHECKER(sequence_checker_);

  // Not owned
  network_time::NetworkTimeTracker* network_time_tracker_ = nullptr;

  base::WeakPtrFactory<NetworkTimeHelper> weak_ptr_factory_{this};
};

}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_NETWORK_TIME_HELPER_H_
