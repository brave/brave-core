/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_P3A_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_P3A_H_

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "base/timer/wall_clock_timer.h"

class PrefService;

namespace playlist {

inline constexpr char kLastUsageTimeHistogramName[] =
    "Brave.Playlist.LastUsageTime";
inline constexpr char kFirstTimeOffsetHistogramName[] =
    "Brave.Playlist.FirstTimeOffset";
inline constexpr char kUsageDaysInWeekHistogramName[] =
    "Brave.Playlist.UsageDaysInWeek";
inline constexpr char kNewUserReturningHistogramName[] =
    "Brave.Playlist.NewUserReturning";

// Manages P3A metrics for playlist
class PlaylistP3A {
 public:
  PlaylistP3A(PrefService* local_state, base::Time browser_first_run_time);
  ~PlaylistP3A();
  PlaylistP3A(const PlaylistP3A&) = delete;
  PlaylistP3A& operator=(PlaylistP3A&) = delete;

  void ReportNewUsage();

 private:
  void SetUpTimer();
  void Update(bool new_usage);

  raw_ptr<PrefService> local_state_;
  base::Time browser_first_run_time_;
  base::WallClockTimer update_timer_;
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_P3A_H_
