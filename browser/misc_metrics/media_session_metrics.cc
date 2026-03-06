/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/media_session_metrics.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/media_session.h"

namespace misc_metrics {

namespace {

constexpr base::TimeDelta kReportInterval = base::Minutes(20);
constexpr base::TimeDelta kMediaPlayingTickInterval = base::Minutes(1);
constexpr base::TimeDelta kFrameDuration = base::Days(7);

constexpr int kMediaSessionUsageBuckets[] = {0, 20, 40, 60, 80, 100};

}  // namespace

MediaSessionMetrics::Session::Session(
    content::MediaSession* media_session,
    PlaybackStateChangedCallback on_playback_state_changed)
    : on_playback_state_changed_(std::move(on_playback_state_changed)) {
  media_session->AddObserver(observer_receiver_.BindNewPipeAndPassRemote());
}

MediaSessionMetrics::Session::~Session() = default;

void MediaSessionMetrics::Session::MediaSessionInfoChanged(
    media_session::mojom::MediaSessionInfoPtr info) {
  bool is_playing = info && info->playback_state ==
                                media_session::mojom::MediaPlaybackState::kPlaying;
  on_playback_state_changed_.Run(is_playing);
}

MediaSessionMetrics::MediaSessionMetrics(PrefService* local_state,
                                         UptimeMonitor* uptime_monitor)
    : local_state_(local_state),
      uptime_monitor_(uptime_monitor),
      weekly_media_storage_(local_state, kMiscMetricsMediaSessionUsageStorage) {
  CHECK(uptime_monitor_);

  frame_start_time_ =
      local_state_->GetTime(kMiscMetricsMediaSessionFrameStartTime);
  if (frame_start_time_.is_null()) {
    ResetFrameStartTime();
  }

  ReportMetric();
}

MediaSessionMetrics::~MediaSessionMetrics() = default;

// static
void MediaSessionMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kMiscMetricsMediaSessionUsageStorage);
  registry->RegisterTimePref(kMiscMetricsMediaSessionFrameStartTime,
                             base::Time());
}

void MediaSessionMetrics::OnMediaSessionCreated(
    content::MediaSession* media_session) {
  if (sessions_.contains(media_session)) {
    return;
  }
  sessions_.emplace(
      media_session,
      std::make_unique<Session>(
          media_session,
          base::BindRepeating(
              &MediaSessionMetrics::OnSessionPlaybackStateChanged,
              base::Unretained(this), media_session)));
}

void MediaSessionMetrics::OnMediaSessionDestroyed(
    content::MediaSession* media_session) {
  RemoveSession(media_session);
}

void MediaSessionMetrics::ResetFrameStartTime() {
  frame_start_time_ = base::Time::Now();
  local_state_->SetTime(kMiscMetricsMediaSessionFrameStartTime,
                        frame_start_time_);
}

void MediaSessionMetrics::OnSessionPlaybackStateChanged(
    content::MediaSession* media_session,
    bool is_playing) {
  if (playing_sessions_.contains(media_session) == is_playing) {
    return;
  }
  if (is_playing) {
    playing_sessions_.insert(media_session);
    StartMediaPlayingTimer();
  } else {
    playing_sessions_.erase(media_session);
  }
}

void MediaSessionMetrics::RemoveSession(content::MediaSession* media_session) {
  sessions_.erase(media_session);
  playing_sessions_.erase(media_session);
}

void MediaSessionMetrics::StartMediaPlayingTimer() {
  if (media_playing_timer_.IsRunning()) {
    return;
  }
  media_playing_timer_.Start(
      FROM_HERE, base::Time::Now() + kMediaPlayingTickInterval,
      base::BindRepeating(&MediaSessionMetrics::OnMediaPlayingTick,
                          base::Unretained(this)));
}

void MediaSessionMetrics::OnMediaPlayingTick() {
  if (playing_sessions_.empty()) {
    return;
  }
  weekly_media_storage_.AddDelta(
      static_cast<uint64_t>(kMediaPlayingTickInterval.InMinutes()));
  StartMediaPlayingTimer();
}

void MediaSessionMetrics::ReportMetric() {
  report_timer_.Start(FROM_HERE, base::Time::Now() + kReportInterval,
                      base::BindRepeating(&MediaSessionMetrics::ReportMetric,
                                          base::Unretained(this)));

  if ((base::Time::Now() - frame_start_time_) < kFrameDuration) {
    return;
  }

  uint64_t uptime_minutes = uptime_monitor_->GetUsedTimeInWeek().InMinutes();
  if (uptime_minutes == 0) {
    return;
  }

  uint64_t media_minutes = weekly_media_storage_.GetWeeklySum();
  int percentage = static_cast<int>(media_minutes * 100 / uptime_minutes);
  if (percentage == 0 && media_minutes > 0) {
    percentage = 1;
  }

  p3a_utils::RecordToHistogramBucket(kMediaSessionUsageHistogramName,
                                     kMediaSessionUsageBuckets, percentage);

  ResetFrameStartTime();
}

}  // namespace misc_metrics
