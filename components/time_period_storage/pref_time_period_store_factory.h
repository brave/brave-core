/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_PREF_TIME_PERIOD_STORE_FACTORY_H_
#define BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_PREF_TIME_PERIOD_STORE_FACTORY_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/components/time_period_storage/time_period_store_factory.h"

class PrefService;
class TimePeriodStore;

// A factory that creates PrefTimePeriodStore which uses PrefService for
// storage.
class PrefTimePeriodStoreFactory : public TimePeriodStoreFactory {
 public:
  PrefTimePeriodStoreFactory(PrefService* prefs, const char* pref_name);

  std::unique_ptr<TimePeriodStore> Create(
      const char* metric_name) const override;

 private:
  const raw_ptr<PrefService> prefs_;
  const char* pref_name_;
};

#endif  // BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_PREF_TIME_PERIOD_STORE_FACTORY_H_
