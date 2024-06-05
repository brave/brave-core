/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_REQUEST_QUEUE_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_REQUEST_QUEUE_H_

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "net/base/backoff_entry.h"

class PrefService;

namespace web_discovery {

class RequestQueue {
 public:
  RequestQueue(
      PrefService* profile_prefs,
      const char* list_pref_name,
      base::TimeDelta request_max_age,
      base::TimeDelta min_request_interval,
      base::TimeDelta max_request_interval,
      size_t max_retries,
      base::RepeatingCallback<void(const base::Value&)> start_request_callback);
  ~RequestQueue();

  RequestQueue(const RequestQueue&) = delete;
  RequestQueue& operator=(const RequestQueue&) = delete;

  void ScheduleRequest(base::Value request_data);
  // Returns data value if request is deleted from queue, due to the retry limit
  // or success
  std::optional<base::Value> NotifyRequestComplete(bool success);

 private:
  void OnFetchTimer();
  void StartFetchTimer(bool use_backoff_delta);

  raw_ptr<PrefService> profile_prefs_;
  const char* list_pref_name_;

  net::BackoffEntry backoff_entry_;

  base::TimeDelta request_max_age_;
  base::TimeDelta min_request_interval_;
  base::TimeDelta max_request_interval_;
  size_t max_retries_;
  base::RepeatingCallback<void(const base::Value&)> start_request_callback_;

  base::OneShotTimer fetch_timer_;
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_REQUEST_QUEUE_H_
