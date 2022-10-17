/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_TIME_PROFILER_TIME_PROFILER_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_TIME_PROFILER_TIME_PROFILER_MANAGER_H_

#include <map>
#include <string>

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

struct TimeProfileInfo;

class TimeProfilerManager final {
 public:
  TimeProfilerManager();
  TimeProfilerManager(const TimeProfilerManager&) = delete;
  TimeProfilerManager& operator=(const TimeProfilerManager&) = delete;
  ~TimeProfilerManager();

  static TimeProfilerManager* GetInstance();

  static bool HasInstance();

  // Begin time profiling and log for the given |category_group| and
  // |pretty_function|. Must be called before any calls to |Measure|, |Reset| or
  // |End|.
  void Begin(const std::string& category_group,
             const std::string& pretty_function);

  // Measure time profiling and log for the given |category_group| and
  // |pretty_function|, line number and an optional message since the last
  // measurement.
  void Measure(const std::string& category_group,
               const std::string& pretty_function,
               const int line,
               const std::string& message = "");

  // End time profiling and log for the given |category_group| and
  // |pretty_function|.
  void End(const std::string& category_group,
           const std::string& pretty_function);

 private:
  std::string BuildSpacesForIndentLevel();

  absl::optional<TimeProfileInfo> GetTimeProfile(
      const std::string& category_group) const;
  bool DoesTimeProfileExist(const std::string& category_group) const;

  std::map<std::string, TimeProfileInfo> time_profile_;

  int indent_level_ = 0;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_TIME_PROFILER_TIME_PROFILER_MANAGER_H_
