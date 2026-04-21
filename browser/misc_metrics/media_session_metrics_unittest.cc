/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/media_session_metrics.h"

#include <memory>
#include <string>

#include "base/test/metrics/histogram_tester.h"
#include "base/test/task_environment.h"
#include "brave/components/misc_metrics/uptime_monitor.h"
#include "brave/components/p3a_utils/custom_attributes.h"
#include "brave/components/p3a_utils/test_event_relay_observer.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/mock_media_session.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/media_session/public/mojom/media_session.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"

namespace misc_metrics {

class MediaSessionMetricsTest : public testing::Test {
 public:
  MediaSessionMetricsTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    MediaSessionMetrics::RegisterPrefs(local_state_.registry());
    metrics_ =
        std::make_unique<MediaSessionMetrics>(&local_state_, &uptime_monitor_);
  }

 protected:
  class MockUptimeMonitor : public UptimeMonitor {
   public:
    base::TimeDelta GetUsedTimeInWeek() const override { return {}; }
    base::WeakPtr<UptimeMonitor> GetWeakPtr() override { return nullptr; }
    bool IsInUse() const override { return is_in_use; }

    bool is_in_use = true;
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
  p3a_utils::TestEventRelayObserver relay_observer_;

  MockUptimeMonitor uptime_monitor_;

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
  EXPECT_EQ(relay_observer_.GetCustomAttribute(kMediaSessionUsageAttributeName),
            std::nullopt);
}

TEST_F(MediaSessionMetricsTest, ZeroPercentBucket) {
  // No media playing → 0 media minutes → 0% → bucket 0.
  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectUniqueSample(kMediaSessionUsageHistogramName, 0, 1);
  EXPECT_EQ(relay_observer_.GetCustomAttribute(kMediaSessionUsageAttributeName),
            "0");
}

TEST_F(MediaSessionMetricsTest, CorrectPercentageBucket) {
  // 40h playing → ~20% → bucket 2. Attribute: 20% → "1-33".
  task_environment_.FastForwardBy(base::Days(1));
  auto* session = AddSession();
  task_environment_.FastForwardBy(base::Hours(40));
  SetPlaying(session, false);

  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_.ExpectUniqueSample(kMediaSessionUsageHistogramName, 2, 1);
  EXPECT_EQ(relay_observer_.GetCustomAttribute(kMediaSessionUsageAttributeName),
            "1-33");
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
  EXPECT_EQ(relay_observer_.GetCustomAttribute(kMediaSessionUsageAttributeName),
            "1-33");
}

TEST_F(MediaSessionMetricsTest, SessionRemovedWhilePlaying) {
  // A session removed while playing still counts its accumulated time.
  task_environment_.FastForwardBy(base::Days(1));
  auto* session = AddSession();
  task_environment_.FastForwardBy(base::Hours(2));
  RemoveSession(session);

  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_.ExpectUniqueSample(kMediaSessionUsageHistogramName, 1, 1);
  // 2h media / ~8 days active ≈ 1% → attribute "1-33".
  EXPECT_EQ(relay_observer_.GetCustomAttribute(kMediaSessionUsageAttributeName),
            "1-33");
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
  EXPECT_EQ(relay_observer_.GetCustomAttribute(kMediaSessionUsageAttributeName),
            "1-33");

  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectBucketCount(kMediaSessionUsageHistogramName, 0, 1);
  histogram_tester_.ExpectTotalCount(kMediaSessionUsageHistogramName, 2);
  EXPECT_EQ(relay_observer_.GetCustomAttribute(kMediaSessionUsageAttributeName),
            "0");
}

TEST_F(MediaSessionMetricsTest, NoActiveTimeAttributeCleared) {
  // Pre-set the attribute to a known value so we can confirm it is cleared.
  p3a_utils::SetCustomAttribute(kMediaSessionUsageAttributeName, "1-33");
  ASSERT_EQ(relay_observer_.GetCustomAttribute(kMediaSessionUsageAttributeName),
            "1-33");

  // Browser never in use and no media playing: active_time stays zero.
  // After the frame elapses the attribute should be cleared to std::nullopt.
  uptime_monitor_.is_in_use = false;
  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectTotalCount(kMediaSessionUsageHistogramName, 0);
  EXPECT_EQ(relay_observer_.GetCustomAttribute(kMediaSessionUsageAttributeName),
            std::nullopt);
}

TEST_F(MediaSessionMetricsTest, ActiveProcessTimeOnlyWhenInUseOrPlaying) {
  // With browser not in use and no media, active process time does not
  // accumulate. Playing media should count toward both storages regardless
  // of IsInUse.
  uptime_monitor_.is_in_use = false;

  // Advance 3 days with no activity — nothing should accumulate.
  task_environment_.FastForwardBy(base::Days(3));

  // Play media for 1 day while not "in use" per uptime monitor.
  auto* session = AddSession();
  task_environment_.FastForwardBy(base::Days(1));
  SetPlaying(session, false);

  // Advance remaining time to trigger report.
  task_environment_.FastForwardBy(base::Days(4));

  // 1 day playing / 1 day active = 100% → bucket 6. Attribute: "68-100".
  histogram_tester_.ExpectUniqueSample(kMediaSessionUsageHistogramName, 6, 1);
  EXPECT_EQ(relay_observer_.GetCustomAttribute(kMediaSessionUsageAttributeName),
            "68-100");
}

}  // namespace misc_metrics
