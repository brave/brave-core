/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads_client/ads_client_notifier_queue.h"

#include <utility>

namespace brave_ads {

AdsClientNotifierQueue::AdsClientNotifierQueue() = default;

AdsClientNotifierQueue::~AdsClientNotifierQueue() = default;

void AdsClientNotifierQueue::Add(base::OnceClosure notifier) {
  queue_.push(std::move(notifier));
}

void AdsClientNotifierQueue::Process() {
  while (!queue_.empty()) {
    std::move(queue_.front()).Run();
    queue_.pop();
  }
}

}  // namespace brave_ads
