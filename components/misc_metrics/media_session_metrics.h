/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_MEDIA_SESSION_METRICS_H_
#define BRAVE_COMPONENTS_MISC_METRICS_MEDIA_SESSION_METRICS_H_

#include <memory>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "base/timer/wall_clock_timer.h"
#include "base/unguessable_token.h"
#include "brave/components/misc_metrics/uptime_monitor.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/media_session/public/mojom/audio_focus.mojom.h"
#include "services/media_session/public/mojom/media_controller.mojom.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

class PrefRegistrySimple;
class PrefService;

namespace misc_metrics {

inline constexpr char kMediaSessionUsageHistogramName[] =
    "Brave.Core.MediaSessionUsage";

// Observes media session activity to track the percentage of active browsing
// time during which media was playing. Reports metrics on a weekly basis.
class MediaSessionMetrics : public media_session::mojom::AudioFocusObserver {
 public:
  MediaSessionMetrics(
      PrefService* local_state,
      UptimeMonitor* uptime_monitor,
      mojo::PendingRemote<media_session::mojom::AudioFocusManager>
          audio_focus_remote,
      mojo::PendingRemote<media_session::mojom::MediaControllerManager>
          controller_manager_remote);
  ~MediaSessionMetrics() override;

  MediaSessionMetrics(const MediaSessionMetrics&) = delete;
  MediaSessionMetrics& operator=(const MediaSessionMetrics&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  // media_session::mojom::AudioFocusObserver:
  void OnFocusGained(
      media_session::mojom::AudioFocusRequestStatePtr state) override;
  void OnFocusLost(
      media_session::mojom::AudioFocusRequestStatePtr state) override;
  void OnRequestIdReleased(const base::UnguessableToken& request_id) override;

 private:
  class Session : public media_session::mojom::MediaControllerObserver {
   public:
    using PlaybackStateChangedCallback = base::RepeatingCallback<void(bool)>;

    Session(mojo::Remote<media_session::mojom::MediaController> controller,
            PlaybackStateChangedCallback on_playback_state_changed);
    ~Session() override;

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    // media_session::mojom::MediaControllerObserver:
    void MediaSessionInfoChanged(
        media_session::mojom::MediaSessionInfoPtr info) override;
    void MediaSessionMetadataChanged(
        const std::optional<media_session::MediaMetadata>& metadata) override {}
    void MediaSessionActionsChanged(
        const std::vector<media_session::mojom::MediaSessionAction>& actions)
        override {}
    void MediaSessionChanged(
        const std::optional<base::UnguessableToken>& request_id) override {}
    void MediaSessionPositionChanged(
        const std::optional<media_session::MediaPosition>& position) override {}

   private:
    PlaybackStateChangedCallback on_playback_state_changed_;
    mojo::Receiver<media_session::mojom::MediaControllerObserver>
        observer_receiver_{this};
  };

  void OnSessionPlaybackStateChanged(const base::UnguessableToken& request_id,
                                     bool is_playing);
  void RemoveSession(const base::UnguessableToken& request_id);
  void OnMediaPlayingTick();
  void StartMediaPlayingTimer();
  void ReportMetric();
  void ResetFrameStartTime();
  void OnGetFocusRequests(
      std::vector<media_session::mojom::AudioFocusRequestStatePtr> requests);

  raw_ptr<PrefService> local_state_;
  raw_ptr<UptimeMonitor> uptime_monitor_;

  absl::flat_hash_map<base::UnguessableToken, std::unique_ptr<Session>>
      sessions_;
  absl::flat_hash_set<base::UnguessableToken> playing_sessions_;

  WeeklyStorage weekly_media_storage_;
  base::Time frame_start_time_;
  base::WallClockTimer report_timer_;
  base::WallClockTimer media_playing_timer_;

  mojo::Remote<media_session::mojom::AudioFocusManager> audio_focus_remote_;
  mojo::Remote<media_session::mojom::MediaControllerManager>
      controller_manager_remote_;
  mojo::Receiver<media_session::mojom::AudioFocusObserver>
      audio_focus_observer_receiver_{this};

  base::WeakPtrFactory<MediaSessionMetrics> weak_ptr_factory_{this};
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_MEDIA_SESSION_METRICS_H_
