/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TIME_PERIOD_STORAGE_PROFILE_ATTRIBUTES_TIME_PERIOD_STORE_H_
#define BRAVE_BROWSER_TIME_PERIOD_STORAGE_PROFILE_ATTRIBUTES_TIME_PERIOD_STORE_H_

#include <string>

#include "base/files/file_path.h"
#include "base/memory/raw_ref.h"
#include "brave/components/time_period_storage/time_period_store.h"

class ProfileAttributesStorage;

namespace base {
class ListValue;
}  // namespace base

// Implementation of TimePeriodStore that uses profile attributes for storage.
//
// Time period values are stored in the `metrics` key of the profile attributes.
// Data is structured as:
// "metrics": {
//   "metric_name_1": {
//     "dict_key_1": [time_period_values]
//     "dict_key_2": [time_period_values]
//   },
//   "metric_name_2": {
//     "dict_key_3": [time_period_values]
//   },
//   ...
// }
class ProfileAttributesTimePeriodStore : public TimePeriodStore {
 public:
  ProfileAttributesTimePeriodStore(
      const base::FilePath& profile_path,
      ProfileAttributesStorage& profile_attributes_storage,
      std::string metric_name,
      std::string dict_key);

  ~ProfileAttributesTimePeriodStore() override;

  ProfileAttributesTimePeriodStore(const ProfileAttributesTimePeriodStore&) =
      delete;
  ProfileAttributesTimePeriodStore& operator=(
      const ProfileAttributesTimePeriodStore&) = delete;

  // TimePeriodStore:
  void Set(base::ListValue data) override;
  const base::ListValue* Get() override;
  void Clear() override;

 private:
  const base::FilePath profile_path_;
  const raw_ref<ProfileAttributesStorage> profile_attributes_storage_;
  const std::string metric_name_;
  const std::string dict_key_;
};

#endif  // BRAVE_BROWSER_TIME_PERIOD_STORAGE_PROFILE_ATTRIBUTES_TIME_PERIOD_STORE_H_
