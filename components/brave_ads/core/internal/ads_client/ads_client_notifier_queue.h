/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_NOTIFIER_QUEUE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_NOTIFIER_QUEUE_H_

#include "base/containers/queue.h"
#include "base/functional/callback.h"

namespace brave_ads {

class AdsClientNotifierQueue final {
 public:
  AdsClientNotifierQueue();

  AdsClientNotifierQueue(const AdsClientNotifierQueue&) = delete;
  AdsClientNotifierQueue& operator=(const AdsClientNotifierQueue&) = delete;

  AdsClientNotifierQueue(AdsClientNotifierQueue&&) noexcept = delete;
  AdsClientNotifierQueue& operator=(AdsClientNotifierQueue&&) noexcept = delete;

  ~AdsClientNotifierQueue();

  void Add(base::OnceClosure notifier);
  void Process();

 private:
  base::queue<base::OnceClosure> queue_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_NOTIFIER_QUEUE_H_
