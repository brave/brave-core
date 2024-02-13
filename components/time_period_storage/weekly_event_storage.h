/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_WEEKLY_EVENT_STORAGE_H_
#define BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_WEEKLY_EVENT_STORAGE_H_

#include <list>
#include <memory>
#include <optional>

#include "base/memory/raw_ref.h"
#include "base/time/clock.h"
#include "base/time/time.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

// WeeklyStorage variant holding a list of events over the past week.
//
// Mostly used by various P3A recorders to report whether an event happened
// during the measurement period.
//
// New event values are recorded by calling `Add()` and are forgotten
// after approximately a week.
//
// Requires |pref_name| to be already registered.
// TODO(djandries): Refactor to extend TimePeriodStorage?
class WeeklyEventStorage {
 public:
  WeeklyEventStorage(PrefService* prefs, const char* pref_name);

  // Accept an explicit clock so tests can manipulate the passage of time.
  WeeklyEventStorage(PrefService* prefs,
                     const char* pref_name,
                     std::unique_ptr<base::Clock> clock);

  ~WeeklyEventStorage();

  WeeklyEventStorage(const WeeklyEventStorage&) = delete;
  WeeklyEventStorage& operator=(const WeeklyEventStorage&) = delete;

  // Add a new event code.
  void Add(int value);
  // Return the most recent event, if any.
  std::optional<int> GetLatest();
  // Check if any events are in the record.
  bool HasEvent();

 private:
  struct Event {
    base::Time day;
    int value = 0;
  };

  void FilterToWeek();

  // Serialize event record to/from a pref.
  void Load();
  void Save();

  const raw_ref<PrefService> prefs_;
  const char* pref_name_ = nullptr;
  std::unique_ptr<base::Clock> clock_;

  std::list<Event> events_;
};

#endif  // BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_WEEKLY_EVENT_STORAGE_H_
