/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_MEDIA_SESSION_METRICS_H_
#define BRAVE_BROWSER_MISC_METRICS_MEDIA_SESSION_METRICS_H_

#include <memory>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/misc_metrics/uptime_monitor.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/media_session/public/mojom/media_session.mojom.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

class PrefRegistrySimple;
class PrefService;

namespace content {
class MediaSession;
}  // namespace content

namespace misc_metrics {

inline constexpr char kMediaSessionUsageHistogramName[] =
    "Brave.Core.MediaSessionUsage";

// Observes media session activity to track the percentage of active browsing
// time during which media was playing. Reports metrics on a weekly basis.
class MediaSessionMetrics {
 public:
  MediaSessionMetrics(PrefService* local_state, UptimeMonitor* uptime_monitor);
  ~MediaSessionMetrics();

  MediaSessionMetrics(const MediaSessionMetrics&) = delete;
  MediaSessionMetrics& operator=(const MediaSessionMetrics&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Called by PageMetricsTabHelper when a new MediaSession is created for a
  // WebContents.
  void OnMediaSessionCreated(content::MediaSession* media_session);

  // Called by PageMetricsTabHelper when a WebContents (and its MediaSession)
  // is being destroyed.
  void OnMediaSessionDestroyed(content::MediaSession* media_session);

 private:
  class Session final : public media_session::mojom::MediaSessionObserver {
   public:
    using PlaybackStateChangedCallback = base::RepeatingCallback<void(bool)>;

    Session(content::MediaSession* media_session,
            PlaybackStateChangedCallback on_playback_state_changed);
    ~Session() override;

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    // media_session::mojom::MediaSessionObserver:
    void MediaSessionInfoChanged(
        media_session::mojom::MediaSessionInfoPtr info) override;
    void MediaSessionMetadataChanged(
        const std::optional<media_session::MediaMetadata>& metadata) override {}
    void MediaSessionActionsChanged(
        const std::vector<media_session::mojom::MediaSessionAction>& actions)
        override {}
    void MediaSessionImagesChanged(
        const base::flat_map<media_session::mojom::MediaSessionImageType,
                             std::vector<media_session::MediaImage>>& images)
        override {}
    void MediaSessionPositionChanged(
        const std::optional<media_session::MediaPosition>& position) override {}

   private:
    PlaybackStateChangedCallback on_playback_state_changed_;
    mojo::Receiver<media_session::mojom::MediaSessionObserver>
        observer_receiver_{this};
  };

  void OnSessionPlaybackStateChanged(content::MediaSession* media_session,
                                     bool is_playing);
  void RemoveSession(content::MediaSession* media_session);
  void OnTick();
  void ReportMetric();
  void ResetFrame();

  raw_ptr<PrefService> local_state_;
  raw_ptr<UptimeMonitor> uptime_monitor_;

  absl::flat_hash_map<content::MediaSession*, std::unique_ptr<Session>>
      sessions_;
  absl::flat_hash_set<content::MediaSession*> playing_sessions_;

  base::Time frame_start_time_;
  base::WallClockTimer report_timer_;
  base::WallClockTimer tick_timer_;

  base::WeakPtrFactory<MediaSessionMetrics> weak_ptr_factory_{this};
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_MEDIA_SESSION_METRICS_H_
