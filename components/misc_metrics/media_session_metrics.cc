/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/media_session_metrics.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace misc_metrics {

namespace {

constexpr base::TimeDelta kReportInterval = base::Minutes(20);
constexpr base::TimeDelta kMediaPlayingTickInterval = base::Minutes(1);
constexpr base::TimeDelta kFrameDuration = base::Days(7);

constexpr int kMediaSessionUsageBuckets[] = {0, 20, 40, 60, 80, 100};

}  // namespace

MediaSessionMetrics::Session::Session(
    mojo::Remote<media_session::mojom::MediaController> controller,
    PlaybackStateChangedCallback on_playback_state_changed)
    : on_playback_state_changed_(std::move(on_playback_state_changed)) {
  controller->AddObserver(observer_receiver_.BindNewPipeAndPassRemote());
}

MediaSessionMetrics::Session::~Session() = default;

void MediaSessionMetrics::Session::MediaSessionInfoChanged(
    media_session::mojom::MediaSessionInfoPtr info) {
  on_playback_state_changed_.Run(
      info && info->playback_state ==
                  media_session::mojom::MediaPlaybackState::kPlaying);
}

MediaSessionMetrics::MediaSessionMetrics(
    PrefService* local_state,
    UptimeMonitor* uptime_monitor,
    mojo::PendingRemote<media_session::mojom::AudioFocusManager>
        audio_focus_remote,
    mojo::PendingRemote<media_session::mojom::MediaControllerManager>
        controller_manager_remote)
    : local_state_(local_state),
      uptime_monitor_(uptime_monitor),
      weekly_media_storage_(local_state, kMiscMetricsMediaSessionUsageStorage) {
  CHECK(uptime_monitor_);
  audio_focus_remote_.Bind(std::move(audio_focus_remote));
  controller_manager_remote_.Bind(std::move(controller_manager_remote));

  frame_start_time_ =
      local_state_->GetTime(kMiscMetricsMediaSessionFrameStartTime);
  if (frame_start_time_.is_null()) {
    ResetFrameStartTime();
  }

  audio_focus_remote_->AddObserver(
      audio_focus_observer_receiver_.BindNewPipeAndPassRemote());

  // Pick up any sessions that were already active before construction.
  audio_focus_remote_->GetFocusRequests(
      base::BindOnce(&MediaSessionMetrics::OnGetFocusRequests,
                     weak_ptr_factory_.GetWeakPtr()));

  ReportMetric();
}

MediaSessionMetrics::~MediaSessionMetrics() = default;

// static
void MediaSessionMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kMiscMetricsMediaSessionUsageStorage);
  registry->RegisterTimePref(kMiscMetricsMediaSessionFrameStartTime,
                             base::Time());
}

void MediaSessionMetrics::ResetFrameStartTime() {
  frame_start_time_ = base::Time::Now();
  local_state_->SetTime(kMiscMetricsMediaSessionFrameStartTime,
                        frame_start_time_);
}

void MediaSessionMetrics::OnGetFocusRequests(
    std::vector<media_session::mojom::AudioFocusRequestStatePtr> requests) {
  for (auto& state : requests) {
    OnFocusGained(std::move(state));
  }
}

void MediaSessionMetrics::OnFocusGained(
    media_session::mojom::AudioFocusRequestStatePtr state) {
  if (!state->request_id.has_value()) {
    return;
  }
  const base::UnguessableToken& request_id = state->request_id.value();
  if (sessions_.contains(request_id)) {
    return;
  }
  mojo::Remote<media_session::mojom::MediaController> controller;
  controller_manager_remote_->CreateMediaControllerForSession(
      controller.BindNewPipeAndPassReceiver(), request_id);
  sessions_.emplace(request_id,
                    std::make_unique<Session>(
                        std::move(controller),
                        base::BindRepeating(
                            &MediaSessionMetrics::OnSessionPlaybackStateChanged,
                            base::Unretained(this), request_id)));
}

void MediaSessionMetrics::OnFocusLost(
    media_session::mojom::AudioFocusRequestStatePtr state) {
  if (!state->request_id.has_value()) {
    return;
  }
  RemoveSession(state->request_id.value());
}

void MediaSessionMetrics::OnRequestIdReleased(
    const base::UnguessableToken& request_id) {
  RemoveSession(request_id);
}

void MediaSessionMetrics::OnSessionPlaybackStateChanged(
    const base::UnguessableToken& request_id,
    bool is_playing) {
  bool was_playing = playing_sessions_.contains(request_id);
  if (is_playing == was_playing) {
    return;
  }
  if (is_playing) {
    playing_sessions_.insert(request_id);
    StartMediaPlayingTimer();
  } else {
    playing_sessions_.erase(request_id);
  }
}

void MediaSessionMetrics::RemoveSession(
    const base::UnguessableToken& request_id) {
  sessions_.erase(request_id);
  playing_sessions_.erase(request_id);
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
