// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <optional>

#include "base/location.h"
#include "base/task/single_thread_task_runner.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/on_device_speech_recognition.mojom.h"
// Private to //content/browser, reachable because this target is listed in
// that target's for_content_tests visibility.
#include "content/browser/speech/speech_recognition_manager_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/speech_recognition_audio_forwarder_config.h"
#include "content/public/browser/speech_recognition_session_config.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_utils.h"
#include "media/audio/audio_system.h"
#include "media/audio/audio_system_impl.h"
#include "media/audio/mock_audio_manager.h"
#include "media/audio/test_audio_thread.h"
#include "media/base/audio_parameters.h"
#include "media/base/media_switches.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

// Brave's engine constructor is the only caller of GetAsrSession, so a request
// for a session is how these tests tell that CreateSession reached it. The
// invalid remote it returns is what the engine ignores rather than binds.
class FakeContentBrowserClient : public ContentBrowserClient {
 public:
  mojo::PendingRemote<local_ai::mojom::AsrSession> GetAsrSession() override {
    requested.SetValue();
    return {};
  }

  // Ready once the engine has asked the embedder for a session.
  base::test::TestFuture<void> requested;
};

// The manager's constructor is protected, for BrowserMainLoop and for the
// upstream fixture named as its friend, so a subclass is how a test of our own
// reaches it.
class TestSpeechRecognitionManagerImpl : public SpeechRecognitionManagerImpl {
 public:
  explicit TestSpeechRecognitionManagerImpl(media::AudioSystem* audio_system)
      : SpeechRecognitionManagerImpl(audio_system, nullptr) {}
};

}  // namespace

// Covers the routing the chromium_src override and the plaster substitutions
// add to CreateSession. The engine's own tests construct the engine directly,
// so they cannot tell whether a session ever reaches it.
//
// The public CreateSession overload cannot be used, because it hardcodes
// can_render_frame_use_on_device to false and so refuses every on-device
// session before one is routed.
class BraveSpeechRecognitionManagerImplTest : public testing::Test {
 protected:
  explicit BraveSpeechRecognitionManagerImplTest(bool brave_engine_enabled) {
    // Upstream's own on-device models are pinned off, so Brave's feature is
    // the only thing that can put a session on the optimization guide path,
    // and with it off the session goes to SODA. Left to their defaults these
    // would decide the branch instead, and this family does flip.
    features_.InitWithFeatureStates(
        {{local_ai::kBraveOnDeviceSpeechRecognition, brave_engine_enabled},
         {media::kOnDeviceWebSpeechGeminiNano, false},
         {media::kOnDeviceWebSpeechSmallExpertModel, false}});
    audio_manager_ = std::make_unique<media::MockAudioManager>(
        std::make_unique<media::TestAudioThread>(true));
    audio_manager_->SetInputStreamParameters(
        media::AudioParameters::UnavailableDeviceParams());
    audio_system_ =
        std::make_unique<media::AudioSystemImpl>(audio_manager_.get());
    manager_ =
        std::make_unique<TestSpeechRecognitionManagerImpl>(audio_system_.get());
  }

  // The manager registers a process wide instance and DCHECKs that only one
  // exists, so it has to be gone before the next test builds its own.
  ~BraveSpeechRecognitionManagerImplTest() override {
    manager_.reset();
    audio_manager_->Shutdown();
  }

  // An on-device session CreateSession will not refuse, so that it gets as far
  // as choosing an engine. With no client to report a refusal to, one would
  // reach the NOTREACHED in the error path rather than pass unnoticed. The
  // forwarder config declares a constructor taking a mutable reference, which
  // leaves its optional neither copyable nor movable, so each call builds that
  // optional in place.
  void CreateOnDeviceSession(SpeechRecognitionAudioForwarderConfig* forwarder) {
    SpeechRecognitionSessionConfig config;
    config.on_device = true;
    config.on_device_available = true;
    config.language = "en-US";
    if (forwarder) {
      manager_->CreateSession(config, mojo::NullReceiver(), mojo::NullRemote(),
                              *forwarder,
                              /*can_render_frame_use_on_device=*/true);
      return;
    }
    manager_->CreateSession(config, mojo::NullReceiver(), mojo::NullRemote(),
                            std::nullopt,
                            /*can_render_frame_use_on_device=*/true);
  }

  base::test::ScopedFeatureList features_;
  BrowserTaskEnvironment task_environment_;
  FakeContentBrowserClient client_;
  ScopedContentBrowserClientSetting client_setting_{&client_};
  std::unique_ptr<media::MockAudioManager> audio_manager_;
  std::unique_ptr<media::AudioSystem> audio_system_;
  std::unique_ptr<TestSpeechRecognitionManagerImpl> manager_;
};

class BraveSpeechRecognitionManagerImplEnabledTest
    : public BraveSpeechRecognitionManagerImplTest {
 protected:
  BraveSpeechRecognitionManagerImplEnabledTest()
      : BraveSpeechRecognitionManagerImplTest(/*brave_engine_enabled=*/true) {}
};

class BraveSpeechRecognitionManagerImplDisabledTest
    : public BraveSpeechRecognitionManagerImplTest {
 protected:
  BraveSpeechRecognitionManagerImplDisabledTest()
      : BraveSpeechRecognitionManagerImplTest(/*brave_engine_enabled=*/false) {}
};

// The preempted IsOptimizationGuideSpeechModel puts an on-device session on the
// optimization guide path, where MakeOnDeviceSpeechEngine builds Brave's engine
// in place of upstream's.
TEST_F(BraveSpeechRecognitionManagerImplEnabledTest,
       OnDeviceSessionUsesBraveEngine) {
  CreateOnDeviceSession(/*forwarder=*/nullptr);
  EXPECT_TRUE(client_.requested.Wait());
}

// A session carrying an audio forwarder goes to the speech recognition service
// process, bypassing the browser and with it Brave's engine, unless it is on
// the optimization guide path. CreateSession recomputes that condition inline,
// so the takeover has to be stated there too.
TEST_F(BraveSpeechRecognitionManagerImplEnabledTest,
       ForwardedAudioSessionUsesBraveEngine) {
  SpeechRecognitionAudioForwarderConfig forwarder(
      mojo::NullReceiver(), /*channel_count=*/1, /*sample_rate=*/16000);
  CreateOnDeviceSession(&forwarder);
  EXPECT_TRUE(client_.requested.Wait());
}

// With the feature off the preemption falls through to upstream's answer.
// Upstream's models are pinned off above, so that answer is false and the
// session goes to SODA, which never asks the embedder for one of Brave's.
TEST_F(BraveSpeechRecognitionManagerImplDisabledTest,
       OnDeviceSessionDoesNotUseBraveEngine) {
  CreateOnDeviceSession(/*forwarder=*/nullptr);

  // Brave's engine posts its request for a session to the UI thread in the
  // constructor while CreateSession runs. A marker posted to that runner
  // afterwards is queued behind it, so once the marker has run the request has
  // too. An unset future at that point means the request was never made rather
  // than not yet arrived.
  base::test::TestFuture<void> drained;
  GetUIThreadTaskRunner({})->PostTask(FROM_HERE, drained.GetCallback());
  ASSERT_TRUE(drained.Wait());
  EXPECT_FALSE(client_.requested.IsReady());
}

}  // namespace content
