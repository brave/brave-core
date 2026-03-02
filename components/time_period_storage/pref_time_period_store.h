/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_PREF_TIME_PERIOD_STORE_H_
#define BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_PREF_TIME_PERIOD_STORE_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/time_period_storage/time_period_store.h"

class PrefService;

namespace base {
class ListValue;
}  // namespace base

// Implementation of TimePeriodStore that uses PrefService for storage.
// Supports both direct list prefs and dictionary-based list prefs.
// |pref_name| must be already registered.
class PrefTimePeriodStore : public TimePeriodStore {
 public:
  // Constructor for direct list pref store.
  PrefTimePeriodStore(PrefService* prefs, const char* pref_name);

  // Constructor for dictionary-based list pref store.
  PrefTimePeriodStore(PrefService* prefs,
                      const char* pref_name,
                      const char* dict_key);

  ~PrefTimePeriodStore() override;

  PrefTimePeriodStore(const PrefTimePeriodStore&) = delete;
  PrefTimePeriodStore& operator=(const PrefTimePeriodStore&) = delete;

  // TimePeriodStore:
  void Set(base::ListValue data) override;
  const base::ListValue* Get() override;
  void Clear() override;

 private:
  const raw_ptr<PrefService> prefs_;
  const char* pref_name_ = nullptr;
  const char* dict_key_ = nullptr;
};

#endif  // BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_PREF_TIME_PERIOD_STORE_H_
