/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_task_queue.h"

#include <utility>

#include "base/check.h"

namespace brave_ads {

AdEventTaskQueue::AdEventTaskQueue() = default;

AdEventTaskQueue::~AdEventTaskQueue() = default;

void AdEventTaskQueue::Add(AdEventTask task, ResultCallback callback) {
  CHECK(task);
  CHECK(callback);

  queue_.push_back(base::BindOnce(
      std::move(task),
      base::BindOnce(&AdEventTaskQueue::OnDidProcessTask,
                     weak_factory_.GetWeakPtr(), std::move(callback))));

  MaybeProcessNextTask();
}

///////////////////////////////////////////////////////////////////////////////

void AdEventTaskQueue::MaybeProcessNextTask() {
  if (is_processing_task_ || queue_.empty()) {
    return;
  }
  is_processing_task_ = true;

  base::OnceClosure task = std::move(queue_.front());
  queue_.pop_front();

  std::move(task).Run();
}

void AdEventTaskQueue::OnDidProcessTask(ResultCallback callback, bool success) {
  is_processing_task_ = false;

  std::move(callback).Run(success);

  MaybeProcessNextTask();
}

}  // namespace brave_ads
