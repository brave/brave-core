/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/media_session_metrics.h"

#include <memory>

#include "base/containers/map_util.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/task_environment.h"
#include "base/unguessable_token.h"
#include "brave/components/misc_metrics/uptime_monitor.h"
#include "components/prefs/testing_pref_service.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/media_session/public/cpp/test/mock_audio_focus_manager.h"
#include "services/media_session/public/cpp/test/mock_media_controller_manager.h"
#include "services/media_session/public/mojom/audio_focus.mojom.h"
#include "services/media_session/public/mojom/media_controller.mojom.h"
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
    testing::Mock::AllowLeak(&audio_focus_manager_);
    testing::Mock::AllowLeak(&controller_manager_);

    EXPECT_CALL(audio_focus_manager_, GetFocusRequests)
        .Times(1)
        .WillOnce([](auto callback) { std::move(callback).Run({}); });

    auto quit = task_environment_.QuitClosure();
    EXPECT_CALL(audio_focus_manager_, AddObserver)
        .Times(1)
        .WillOnce([this, quit = std::move(quit)](auto pending_observer) {
          audio_focus_observer_.Bind(std::move(pending_observer));
          quit.Run();
        });

    MediaSessionMetrics::RegisterPrefs(local_state_.registry());

    metrics_ = std::make_unique<MediaSessionMetrics>(
        &local_state_, &uptime_monitor_,
        audio_focus_manager_.GetPendingRemote(),
        controller_manager_.GetPendingRemote());
    task_environment_.RunUntilQuit();
  }

 protected:
  class MockUptimeMonitor : public UptimeMonitor {
   public:
    base::TimeDelta GetUsedTimeInWeek() const override {
      return base::Time::Now() - start_time_;
    }
    base::WeakPtr<UptimeMonitor> GetWeakPtr() override { return nullptr; }
    bool IsInUse() const override { return true; }

   private:
    base::Time start_time_ = base::Time::Now();
  };

  class MockMediaController : public media_session::mojom::MediaController {
   public:
    MockMediaController(
        mojo::PendingReceiver<media_session::mojom::MediaController> receiver,
        base::OnceClosure on_observer_bound)
        : receiver_(this, std::move(receiver)),
          on_observer_bound_(std::move(on_observer_bound)) {}

    // media_session::mojom::MediaController:
    void AddObserver(
        mojo::PendingRemote<media_session::mojom::MediaControllerObserver> obs)
        override {
      observer.Bind(std::move(obs));
      std::move(on_observer_bound_).Run();
    }
    void Suspend() override {}
    void Resume() override {}
    void Stop() override {}
    void ToggleSuspendResume() override {}
    void PreviousTrack() override {}
    void NextTrack() override {}
    void SkipAd() override {}
    void Seek(base::TimeDelta) override {}
    void SeekTo(base::TimeDelta) override {}
    void ScrubTo(base::TimeDelta) override {}
    void EnterPictureInPicture() override {}
    void ExitPictureInPicture() override {}
    void ObserveImages(
        media_session::mojom::MediaSessionImageType,
        int32_t,
        int32_t,
        mojo::PendingRemote<media_session::mojom::MediaControllerImageObserver>)
        override {}
    void SetAudioSinkId(const std::optional<std::string>&) override {}
    void ToggleMicrophone() override {}
    void ToggleCamera() override {}
    void HangUp() override {}
    void Raise() override {}
    void SetMute(bool) override {}
    void RequestMediaRemoting() override {}
    void EnterAutoPictureInPicture() override {}

    mojo::Remote<media_session::mojom::MediaControllerObserver> observer;

   private:
    mojo::Receiver<media_session::mojom::MediaController> receiver_;
    base::OnceClosure on_observer_bound_;
  };

  base::UnguessableToken AddSession() {
    auto id = base::UnguessableToken::Create();
    auto quit = task_environment_.QuitClosure();
    EXPECT_CALL(controller_manager_, CreateMediaControllerForSession)
        .WillOnce([this, quit](auto receiver, auto session_id) {
          mock_controllers_.emplace(
              session_id,
              std::make_unique<MockMediaController>(std::move(receiver), quit));
        });
    auto state = media_session::mojom::AudioFocusRequestState::New();
    state->request_id = id;
    state->session_info = media_session::mojom::MediaSessionInfo::New();
    audio_focus_observer_->OnFocusGained(std::move(state));
    task_environment_.RunUntilQuit();
    SetPlaying(id, true);
    return id;
  }

  void RemoveSession(const base::UnguessableToken& id) {
    audio_focus_observer_->OnRequestIdReleased(id);
  }

  MockMediaController* GetController(const base::UnguessableToken& id) {
    auto* entry = base::FindOrNull(mock_controllers_, id);
    return entry ? entry->get() : nullptr;
  }

  void SetPlaying(const base::UnguessableToken& id, bool playing) {
    MockMediaController* controller = GetController(id);
    ASSERT_TRUE(controller);
    auto info = media_session::mojom::MediaSessionInfo::New();
    info->playback_state =
        playing ? media_session::mojom::MediaPlaybackState::kPlaying
                : media_session::mojom::MediaPlaybackState::kPaused;
    controller->observer->MediaSessionInfoChanged(std::move(info));
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;

  MockUptimeMonitor uptime_monitor_;

  testing::NiceMock<media_session::test::MockAudioFocusManager>
      audio_focus_manager_;
  testing::NiceMock<media_session::test::MockMediaControllerManager>
      controller_manager_;

  mojo::Remote<media_session::mojom::AudioFocusObserver> audio_focus_observer_;
  absl::flat_hash_map<base::UnguessableToken,
                      std::unique_ptr<MockMediaController>>
      mock_controllers_;

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
  // 40h = → ~20% → bucket 2.
  task_environment_.FastForwardBy(base::Days(1));
  auto id = AddSession();
  task_environment_.FastForwardBy(base::Hours(40));
  SetPlaying(id, false);

  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_.ExpectUniqueSample(kMediaSessionUsageHistogramName, 2, 1);
}

TEST_F(MediaSessionMetricsTest, SimultaneousSessionsNoDuplication) {
  // Two sessions playing simultaneously for 40h counts as 2400 min, not 4800.
  task_environment_.FastForwardBy(base::Days(1));
  auto id1 = AddSession();
  auto id2 = AddSession();

  task_environment_.FastForwardBy(base::Hours(40));
  SetPlaying(id1, false);
  SetPlaying(id2, false);

  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_.ExpectUniqueSample(kMediaSessionUsageHistogramName, 2, 1);
}

TEST_F(MediaSessionMetricsTest, SessionRemovedWhilePlaying) {
  // A session removed while playing still counts its accumulated time.
  task_environment_.FastForwardBy(base::Days(1));
  auto id = AddSession();
  task_environment_.FastForwardBy(base::Hours(2));
  RemoveSession(id);

  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_.ExpectUniqueSample(kMediaSessionUsageHistogramName, 1, 1);
}

TEST_F(MediaSessionMetricsTest, FrameResetAfterReport) {
  // First week: 1h media / 8 days uptime → bucket 1.
  // Second week: no media → bucket 0.
  task_environment_.FastForwardBy(base::Days(1));
  auto id = AddSession();
  task_environment_.FastForwardBy(base::Hours(1));
  SetPlaying(id, false);

  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectBucketCount(kMediaSessionUsageHistogramName, 1, 1);

  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectBucketCount(kMediaSessionUsageHistogramName, 0, 1);
  histogram_tester_.ExpectTotalCount(kMediaSessionUsageHistogramName, 2);
}

}  // namespace misc_metrics
