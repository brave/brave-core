/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ctime>

#include "bat/ads/internal/time.h"

namespace ads {

std::string Time::Timestamp() {
  time_t rawtime;
  std::time(&rawtime);

  char buffer[24];
  struct tm* timeinfo = std::localtime(&rawtime);
  strftime(buffer, 24, "%FT%TZ", timeinfo);
  return std::string(buffer);
}

uint64_t Time::NowInSeconds() {
  auto now = base::Time::Now();
  return now.ToDoubleT();
}

uint64_t Time::MigrateTimestampToDoubleT(const uint64_t timestamp_in_seconds) {
  if (timestamp_in_seconds < 10000000000) {
    // Already migrated as DoubleT will never reach 10000000000 in our lifetime
    // and legacy timestamps are above 10000000000
    return timestamp_in_seconds;
  }

  // Migrate date to DoubleT
  auto now = base::Time::Now();
  auto now_in_seconds = static_cast<uint64_t>((now - base::Time()).InSeconds());

  auto delta = timestamp_in_seconds - now_in_seconds;

  auto date = now + base::TimeDelta::FromSeconds(delta);
  return date.ToDoubleT();
}

base::Time Time::FromDoubleT(const uint64_t timestamp_in_seconds) {
  if (timestamp_in_seconds == 0) {
    // Workaround for Windows crash when passing 0 to LocalExplode
    return base::Time::Now();
  }

  return base::Time::FromDoubleT(timestamp_in_seconds);
}

}  // namespace ads
