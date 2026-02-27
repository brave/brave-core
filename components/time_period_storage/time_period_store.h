/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_TIME_PERIOD_STORE_H_
#define BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_TIME_PERIOD_STORE_H_

namespace base {
class ListValue;
}  // namespace base

// An interface for classes that store list of time period values for a
// TimePeriodStorage.
class TimePeriodStore {
 public:
  virtual ~TimePeriodStore() = default;

  // Returns a pointer to a list of time period values. Returned pointer
  // shouldn't be cached because it may be invalidated by the time it's used.
  virtual const base::ListValue* Get() = 0;

  // Sets a list of time period values.
  virtual void Set(base::ListValue data) = 0;

  // Clears the list of time period values.
  virtual void Clear() = 0;
};

#endif  // BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_TIME_PERIOD_STORE_H_
