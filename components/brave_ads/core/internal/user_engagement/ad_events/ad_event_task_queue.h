/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_TASK_QUEUE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_TASK_QUEUE_H_

#include "base/containers/circular_deque.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads {

using AdEventTask = base::OnceCallback<void(ResultCallback callback)>;

// Triggers ad events in a queue, so each ad event observes the results of the
// previous ones.
class AdEventTaskQueue final {
 public:
  AdEventTaskQueue();

  AdEventTaskQueue(const AdEventTaskQueue&) = delete;
  AdEventTaskQueue& operator=(const AdEventTaskQueue&) = delete;

  ~AdEventTaskQueue();

  void Add(AdEventTask task, ResultCallback callback);

 private:
  void MaybeProcessNextTask();
  void OnDidProcessTask(ResultCallback callback, bool success);

  base::circular_deque<base::OnceClosure> queue_;
  bool is_processing_task_ = false;

  base::WeakPtrFactory<AdEventTaskQueue> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_TASK_QUEUE_H_
