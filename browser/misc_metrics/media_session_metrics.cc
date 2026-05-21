/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/media_session_metrics.h"

#include <array>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/misc_metrics/uptime_monitor.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/p3a_utils/custom_attributes.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/media_session.h"

namespace misc_metrics {

namespace {

constexpr base::TimeDelta kReportInterval = base::Minutes(20);
constexpr base::TimeDelta kTickInterval = base::Seconds(30);
constexpr base::TimeDelta kFrameDuration = base::Days(7);

constexpr int kMediaSessionUsageBuckets[] = {0, 20, 40, 60, 80, 95};
constexpr int kMediaSessionUsageAttributeThresholds[] = {0, 33, 67};
constexpr std::array<std::string_view, 4> kMediaSessionUsageAttributeValues = {
    "0", "1-33", "34-67", "68-100"};

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
  bool is_playing =
      info && info->playback_state ==
                  media_session::mojom::MediaPlaybackState::kPlaying;
  on_playback_state_changed_.Run(is_playing);
}

MediaSessionMetrics::MediaSessionMetrics(PrefService* local_state,
                                         UptimeMonitor* uptime_monitor)
    : local_state_(local_state), uptime_monitor_(uptime_monitor) {
  frame_start_time_ =
      local_state_->GetTime(kMiscMetricsMediaSessionFrameStartTime);
  if (frame_start_time_.is_null()) {
    ResetFrame();
  }

  tick_timer_.Start(
      FROM_HERE, base::Time::Now() + kTickInterval,
      base::BindOnce(&MediaSessionMetrics::OnTick, base::Unretained(this)));

  ReportMetric();
}

MediaSessionMetrics::~MediaSessionMetrics() = default;

// static
void MediaSessionMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterTimeDeltaPref(kMiscMetricsMediaSessionPlayingTime, {});
  registry->RegisterTimeDeltaPref(kMiscMetricsMediaSessionActiveProcessTime,
                                  {});
  registry->RegisterTimePref(kMiscMetricsMediaSessionFrameStartTime,
                             base::Time());
}

void MediaSessionMetrics::OnMediaSessionCreated(
    content::MediaSession* media_session) {
  if (sessions_.contains(media_session)) {
    return;
  }
  sessions_.emplace(media_session,
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

void MediaSessionMetrics::ResetFrame() {
  frame_start_time_ = base::Time::Now();
  local_state_->SetTime(kMiscMetricsMediaSessionFrameStartTime,
                        frame_start_time_);
  local_state_->SetTimeDelta(kMiscMetricsMediaSessionPlayingTime, {});
  local_state_->SetTimeDelta(kMiscMetricsMediaSessionActiveProcessTime, {});
}

void MediaSessionMetrics::OnSessionPlaybackStateChanged(
    content::MediaSession* media_session,
    bool is_playing) {
  bool was_playing = playing_sessions_.contains(media_session);
  if (is_playing == was_playing) {
    return;
  }
  if (is_playing) {
    playing_sessions_.insert(media_session);
  } else {
    playing_sessions_.erase(media_session);
  }
}

void MediaSessionMetrics::RemoveSession(content::MediaSession* media_session) {
  sessions_.erase(media_session);
  playing_sessions_.erase(media_session);
}

void MediaSessionMetrics::OnTick() {
  bool is_playing = !playing_sessions_.empty();
  if (is_playing || uptime_monitor_->IsInUse()) {
    local_state_->SetTimeDelta(
        kMiscMetricsMediaSessionActiveProcessTime,
        local_state_->GetTimeDelta(kMiscMetricsMediaSessionActiveProcessTime) +
            kTickInterval);
  }
  if (is_playing) {
    local_state_->SetTimeDelta(
        kMiscMetricsMediaSessionPlayingTime,
        local_state_->GetTimeDelta(kMiscMetricsMediaSessionPlayingTime) +
            kTickInterval);
  }
  tick_timer_.Start(
      FROM_HERE, base::Time::Now() + kTickInterval,
      base::BindOnce(&MediaSessionMetrics::OnTick, base::Unretained(this)));
}

void MediaSessionMetrics::ReportMetric() {
  report_timer_.Start(FROM_HERE, base::Time::Now() + kReportInterval,
                      base::BindOnce(&MediaSessionMetrics::ReportMetric,
                                     base::Unretained(this)));

  if ((base::Time::Now() - frame_start_time_) < kFrameDuration) {
    return;
  }

  base::TimeDelta active_time =
      local_state_->GetTimeDelta(kMiscMetricsMediaSessionActiveProcessTime);
  if (active_time.is_zero()) {
    p3a_utils::SetCustomAttribute(kMediaSessionUsageAttributeName,
                                  std::nullopt);
    return;
  }

  base::TimeDelta media_time =
      local_state_->GetTimeDelta(kMiscMetricsMediaSessionPlayingTime);
  int percentage = static_cast<int>(media_time * 100 / active_time);
  if (percentage == 0 && !media_time.is_zero()) {
    percentage = 1;
  }

  p3a_utils::SetCustomAttribute(
      kMediaSessionUsageAttributeName,
      kMediaSessionUsageAttributeValues.at(p3a_utils::BucketIndex(
          kMediaSessionUsageAttributeThresholds, percentage)));

  p3a_utils::RecordToHistogramBucket(kMediaSessionUsageHistogramName,
                                     kMediaSessionUsageBuckets, percentage);

  ResetFrame();
}

}  // namespace misc_metrics
