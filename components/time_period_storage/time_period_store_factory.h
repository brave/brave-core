/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_TIME_PERIOD_STORE_FACTORY_H_
#define BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_TIME_PERIOD_STORE_FACTORY_H_

#include <memory>

class TimePeriodStore;

// Factory which abstracts the creation of TimePeriodStore instances.
class TimePeriodStoreFactory {
 public:
  virtual ~TimePeriodStoreFactory() = default;

  virtual std::unique_ptr<TimePeriodStore> Build(
      const char* metric_name) const = 0;
};

#endif  // BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_TIME_PERIOD_STORE_FACTORY_H_
