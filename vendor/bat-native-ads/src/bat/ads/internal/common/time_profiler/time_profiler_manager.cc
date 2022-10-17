/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/time_profiler/time_profiler_manager.h"

#include "base/check_op.h"
#include "base/time/time.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/common/time_profiler/pretty_function_parser_util.h"
#include "bat/ads/internal/common/time_profiler/time_profile_info.h"
#include "bat/ads/internal/common/time_profiler/time_profiler_manager_duration_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

namespace {

TimeProfilerManager* g_time_profiler_manager = nullptr;

constexpr int kIndentSize = 2;

}  // namespace

TimeProfilerManager::TimeProfilerManager() {
  DCHECK(!g_time_profiler_manager);
  g_time_profiler_manager = this;
}

TimeProfilerManager::~TimeProfilerManager() {
  DCHECK_EQ(this, g_time_profiler_manager);
  g_time_profiler_manager = nullptr;
}

// static
TimeProfilerManager* TimeProfilerManager::GetInstance() {
  DCHECK(g_time_profiler_manager);
  return g_time_profiler_manager;
}

// static
bool TimeProfilerManager::HasInstance() {
  return !!g_time_profiler_manager;
}

void TimeProfilerManager::Begin(const std::string& category_group,
                                const std::string& pretty_function) {
  DCHECK(!category_group.empty()) << "Category group must be specified";
  DCHECK(!pretty_function.empty());

  DCHECK(!DoesTimeProfileExist(category_group))
      << "Begin() already called for " << category_group;

  TimeProfileInfo time_profile;
  time_profile.indent_level = indent_level_;
  time_profile.name = ParseFunctionFromPrettyFunction(pretty_function);
  time_profile.start_time_ticks = base::TimeTicks::Now();
  time_profile.last_time_ticks = time_profile.start_time_ticks;
  time_profile_[category_group] = time_profile;

  BLOG(6, BuildSpacesForIndentLevel()
              << "TimeProfiler.Begin [" << category_group << "]");

  indent_level_++;
}

void TimeProfilerManager::Measure(const std::string& category_group,
                                  const std::string& pretty_function,
                                  const int line,
                                  const std::string& message) {
  DCHECK(!category_group.empty()) << "Category group must be specified";
  DCHECK(!pretty_function.empty());

  absl::optional<TimeProfileInfo> time_profile = GetTimeProfile(category_group);
  DCHECK(time_profile) << R"(You must call Begin(")" << category_group
                       << R"(") before Measure(")" << category_group << R"("))";

  const std::string log_message = BuildDurationSinceLastTimeTicksLogMessage(
      category_group, line, message, *time_profile);
  BLOG(6, BuildSpacesForIndentLevel() << log_message);

  time_profile->last_time_ticks = base::TimeTicks::Now();

  time_profile_[category_group] = *time_profile;
}

void TimeProfilerManager::End(const std::string& category_group,
                              const std::string& pretty_function) {
  DCHECK(!category_group.empty()) << "Category group must be specified";
  DCHECK(!pretty_function.empty());

  const absl::optional<TimeProfileInfo> time_profile =
      GetTimeProfile(category_group);
  DCHECK(time_profile) << R"(You must call Begin(")" << category_group
                       << R"(") before End(")" << category_group << R"("))";

  indent_level_--;

  const std::string duration =
      GetDurationSinceLastTimeTicks(time_profile->start_time_ticks);

  BLOG(6, BuildSpacesForIndentLevel()
              << "TimeProfiler.End [" << category_group << "]: " << duration);

  time_profile_.erase(category_group);
}

///////////////////////////////////////////////////////////////////////////////

std::string TimeProfilerManager::BuildSpacesForIndentLevel() {
  return std::string(indent_level_ * kIndentSize, ' ');
}

absl::optional<TimeProfileInfo> TimeProfilerManager::GetTimeProfile(
    const std::string& category_group) const {
  DCHECK(!category_group.empty());

  if (time_profile_.find(category_group) == time_profile_.end()) {
    return absl::nullopt;
  }

  return time_profile_.at(category_group);
}

bool TimeProfilerManager::DoesTimeProfileExist(
    const std::string& category_group) const {
  DCHECK(!category_group.empty());

  return !!GetTimeProfile(category_group);
}

}  // namespace ads
