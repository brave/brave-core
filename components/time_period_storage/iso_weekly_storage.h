/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_ISO_WEEKLY_STORAGE_H_
#define BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_ISO_WEEKLY_STORAGE_H_

#include "brave/components/time_period_storage/time_period_storage.h"

class PrefService;

class ISOWeeklyStorage : public TimePeriodStorage {
 public:
  ISOWeeklyStorage(PrefService* prefs, const char* pref_name);

  ISOWeeklyStorage(const ISOWeeklyStorage&) = delete;
  ISOWeeklyStorage& operator=(const ISOWeeklyStorage&) = delete;

  uint64_t GetLastISOWeekSum() const;
  uint64_t GetCurrentISOWeekSum() const;

 private:
  base::Time GetLastMondayTime(int week_offset) const;
};

#endif  // BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_ISO_WEEKLY_STORAGE_H_
