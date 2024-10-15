/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/functional/once_closure_queue.h"

#include <utility>

namespace brave_ads {

OnceClosureQueue::OnceClosureQueue() = default;

OnceClosureQueue::~OnceClosureQueue() = default;

void OnceClosureQueue::Add(base::OnceClosure closure) {
  queue_.push(std::move(closure));
}

void OnceClosureQueue::Process() {
  while (!queue_.empty()) {
    std::move(queue_.front()).Run();
    queue_.pop();
  }
}

}  // namespace brave_ads
