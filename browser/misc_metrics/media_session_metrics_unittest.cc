/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/media_session_metrics.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/test/task_environment.h"
#include "brave/components/misc_metrics/uptime_monitor.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/mock_media_session.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/media_session/public/mojom/media_session.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"

namespace misc_metrics {

namespace {
constexpr char kTestUptimeStoragePref[] = "test.uptime_storage";
}  // namespace

class MediaSessionMetricsTest : public testing::Test {
 public:
  MediaSessionMetricsTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    MediaSessionMetrics::RegisterPrefs(local_state_.registry());
    local_state_.registry()->RegisterListPref(kTestUptimeStoragePref);
    uptime_monitor_ = std::make_unique<MockUptimeMonitor>(&local_state_);
    metrics_ =
        std::make_unique<MediaSessionMetrics>(&local_state_, uptime_monitor_.get());
  }

 protected:
  class MockUptimeMonitor : public UptimeMonitor {
   public:
    explicit MockUptimeMonitor(PrefService* prefs)
        : storage_(prefs, kTestUptimeStoragePref, 8) {}
    base::TimeDelta GetUsedTimeInWeek() const override { return {}; }
    TimePeriodStorage* GetTimePeriodStorage() override {
      const base::Time now = base::Time::Now();
      storage_.AddDelta((now - frame_start_time_).InSeconds());
      frame_start_time_ = now;
      return &storage_;
    }
    base::WeakPtr<UptimeMonitor> GetWeakPtr() override { return nullptr; }
    bool IsInUse() const override { return true; }

   private:
    base::Time frame_start_time_ = base::Time::Now();
    TimePeriodStorage storage_;
  };

  content::MockMediaSession* AddSession() {
    auto session = std::make_unique<content::MockMediaSession>();
    content::MockMediaSession* session_ptr = session.get();

    auto quit = task_environment_.QuitClosure();
    EXPECT_CALL(*session_ptr, AddObserver)
        .WillOnce([this, &session_ptr,
                   quit = std::move(quit)](auto pending_observer) {
          session_observers_[session_ptr].Bind(std::move(pending_observer));
          quit.Run();
        });

    mock_sessions_.push_back(std::move(session));
    metrics_->OnMediaSessionCreated(session_ptr);
    task_environment_.RunUntilQuit();

    SetPlaying(session_ptr, true);
    return session_ptr;
  }

  void RemoveSession(content::MockMediaSession* session) {
    metrics_->OnMediaSessionDestroyed(session);
  }

  void SetPlaying(content::MockMediaSession* session, bool playing) {
    auto info = media_session::mojom::MediaSessionInfo::New();
    info->playback_state =
        playing ? media_session::mojom::MediaPlaybackState::kPlaying
                : media_session::mojom::MediaPlaybackState::kPaused;
    session_observers_[session]->MediaSessionInfoChanged(std::move(info));
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;

  std::unique_ptr<MockUptimeMonitor> uptime_monitor_;

  std::vector<std::unique_ptr<content::MockMediaSession>> mock_sessions_;
  absl::flat_hash_map<content::MockMediaSession*,
                      mojo::Remote<media_session::mojom::MediaSessionObserver>>
      session_observers_;

  std::unique_ptr<MediaSessionMetrics> metrics_;
};

TEST_F(MediaSessionMetricsTest, NoReportBeforeOneWeek) {
  AddSession();
  task_environment_.FastForwardBy(base::Days(6));
  histogram_tester_.ExpectTotalCount(kMediaSessionUsageHistogramName, 0);
}

TEST_F(MediaSessionMetricsTest, ZeroPercentBucket) {
  // No media playing → 0 media minutes → 0% → bucket 0.
  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectUniqueSample(kMediaSessionUsageHistogramName, 0, 1);
}

TEST_F(MediaSessionMetricsTest, CorrectPercentageBucket) {
  // 40h playing → ~20% → bucket 2.
  task_environment_.FastForwardBy(base::Days(1));
  auto* session = AddSession();
  task_environment_.FastForwardBy(base::Hours(40));
  SetPlaying(session, false);

  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_.ExpectUniqueSample(kMediaSessionUsageHistogramName, 2, 1);
}

TEST_F(MediaSessionMetricsTest, SimultaneousSessionsNoDuplication) {
  // Two sessions playing simultaneously for 40h counts as 2400 min, not 4800.
  task_environment_.FastForwardBy(base::Days(1));
  auto* session1 = AddSession();
  auto* session2 = AddSession();

  task_environment_.FastForwardBy(base::Hours(40));
  SetPlaying(session1, false);
  SetPlaying(session2, false);

  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_.ExpectUniqueSample(kMediaSessionUsageHistogramName, 2, 1);
}

TEST_F(MediaSessionMetricsTest, SessionRemovedWhilePlaying) {
  // A session removed while playing still counts its accumulated time.
  task_environment_.FastForwardBy(base::Days(1));
  auto* session = AddSession();
  task_environment_.FastForwardBy(base::Hours(2));
  RemoveSession(session);

  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_.ExpectUniqueSample(kMediaSessionUsageHistogramName, 1, 1);
}

TEST_F(MediaSessionMetricsTest, FrameResetAfterReport) {
  // First week: 1h media / 8 days uptime → bucket 1.
  // Second week: no media → bucket 0.
  task_environment_.FastForwardBy(base::Days(1));
  auto* session = AddSession();
  task_environment_.FastForwardBy(base::Hours(1));
  SetPlaying(session, false);

  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectBucketCount(kMediaSessionUsageHistogramName, 1, 1);

  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectBucketCount(kMediaSessionUsageHistogramName, 0, 1);
  histogram_tester_.ExpectTotalCount(kMediaSessionUsageHistogramName, 2);
}

}  // namespace misc_metrics
