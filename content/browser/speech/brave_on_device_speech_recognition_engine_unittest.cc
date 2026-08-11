// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "brave/components/local_ai/core/on_device_speech_recognition.mojom.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/speech_recognition_session_config.h"
#include "content/public/common/content_client.h"
#include "content/public/test/test_renderer_host.h"
#include "media/base/audio_parameters.h"
#include "media/base/channel_layout.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/on_device_model/public/mojom/on_device_model.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

// These two are private to //content/browser and can be used only from
// content_unittests, but Brave has no such target, so we work around it with
// `nogncheck`.
#include "brave/content/browser/speech/brave_on_device_speech_recognition_engine.h"  // nogncheck
#include "content/browser/speech/speech_recognition_engine.h"  // nogncheck

namespace content {

namespace {

constexpr int kSampleRateHz = 16000;

// Stand-in for Brave's worker. Records what the engine asked for and lets a
// test see when the engine drops either pipe.
class FakeAsrSession : public local_ai::mojom::AsrSession,
                       public on_device_model::mojom::AsrStreamInput {
 public:
  FakeAsrSession() = default;
  ~FakeAsrSession() override = default;

  mojo::PendingRemote<local_ai::mojom::AsrSession> BindRemote() {
    return session_receiver_.BindNewPipeAndPassRemote();
  }

  // local_ai::mojom::AsrSession:
  void Start(
      on_device_model::mojom::AsrStreamOptionsPtr options,
      mojo::PendingReceiver<on_device_model::mojom::AsrStreamInput> stream,
      mojo::PendingRemote<on_device_model::mojom::AsrStreamResponder> responder)
      override {
    ++start_count_;
    sample_rate_hz_ = options->sample_rate_hz;
    stream_receiver_.Bind(std::move(stream));
    responder_.Bind(std::move(responder));
  }

  // on_device_model::mojom::AsrStreamInput:
  void AddAudioChunk(on_device_model::mojom::AudioDataPtr data) override {}

  int start_count() const { return start_count_; }
  uint32_t sample_rate_hz() const { return sample_rate_hz_; }
  bool stream_connected() const { return stream_receiver_.is_bound(); }
  bool responder_connected() const { return responder_.is_connected(); }
  bool session_connected() const { return session_receiver_.is_bound(); }

  // Reflects the engine dropping a pipe, which only reaches us once the
  // message loop has run.
  void set_stream_disconnect_handler(base::OnceClosure closure) {
    stream_receiver_.set_disconnect_handler(std::move(closure));
  }
  void set_session_disconnect_handler(base::OnceClosure closure) {
    session_receiver_.set_disconnect_handler(std::move(closure));
  }

 private:
  int start_count_ = 0;
  uint32_t sample_rate_hz_ = 0;
  mojo::Receiver<local_ai::mojom::AsrSession> session_receiver_{this};
  mojo::Receiver<on_device_model::mojom::AsrStreamInput> stream_receiver_{this};
  mojo::Remote<on_device_model::mojom::AsrStreamResponder> responder_;
};

// Hands out sessions from the fake worker, or refuses, the way
// BraveContentBrowserClient does when the feature is off or no model is
// installed.
class FakeContentBrowserClient : public ContentBrowserClient {
 public:
  explicit FakeContentBrowserClient(FakeAsrSession* session)
      : session_(session) {}

  mojo::PendingRemote<local_ai::mojom::AsrSession> GetAsrSession(
      BrowserContext* browser_context) override {
    return session_ ? session_->BindRemote()
                    : mojo::PendingRemote<local_ai::mojom::AsrSession>();
  }

 private:
  raw_ptr<FakeAsrSession> session_;
};

// Records what the engine reported upward. SpeechRecognizerImpl is the real
// delegate, and an empty result set is how it is told to end recognition.
class TestDelegate : public SpeechRecognitionEngine::Delegate {
 public:
  void OnSpeechRecognitionEngineResults(
      const std::vector<media::mojom::WebSpeechRecognitionResultPtr>& results)
      override {
    ++results_count_;
    last_results_empty_ = results.empty();
  }
  void OnSpeechRecognitionEngineEndOfUtterance() override {}
  void OnSpeechRecognitionEngineError(
      const media::mojom::SpeechRecognitionError& error) override {
    ++error_count_;
  }

  int results_count() const { return results_count_; }
  int error_count() const { return error_count_; }
  bool last_results_empty() const { return last_results_empty_; }

 private:
  int results_count_ = 0;
  int error_count_ = 0;
  bool last_results_empty_ = false;
};

}  // namespace

class BraveOnDeviceSpeechRecognitionEngineTest
    : public RenderViewHostTestHarness {
 public:
  void TearDown() override {
    if (old_client_) {
      SetBrowserClientForTesting(old_client_);
      old_client_ = nullptr;
    }
    engine_.reset();
    RenderViewHostTestHarness::TearDown();
  }

 protected:
  // Builds the engine against a client that hands out `session`, or refuses
  // when it is null. The session is asked for on the UI thread, so it has not
  // arrived when this returns.
  void CreateEngine(FakeAsrSession* session) {
    client_ = std::make_unique<FakeContentBrowserClient>(session);
    old_client_ = SetBrowserClientForTesting(client_.get());

    SpeechRecognitionSessionConfig config;
    // The engine resolves a frame by id on the UI thread before it asks the
    // embedder for a session, so this has to name a live one.
    config.initial_context.global_id = main_rfh()->GetGlobalId();
    engine_ = std::make_unique<BraveOnDeviceSpeechRecognitionEngine>(config);
    engine_->set_delegate(&delegate_);
  }

  void SetAudioParameters() {
    engine_->SetAudioParameters(
        media::AudioParameters(media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
                               media::ChannelLayoutConfig::Mono(),
                               kSampleRateHz, kSampleRateHz / 100));
  }

  TestDelegate delegate_;
  std::unique_ptr<BraveOnDeviceSpeechRecognitionEngine> engine_;

 private:
  std::unique_ptr<FakeContentBrowserClient> client_;
  raw_ptr<ContentBrowserClient> old_client_ = nullptr;
};

// The worker stream needs the session remote and the audio parameters, which
// arrive asynchronously and in either order, so each order has to end in
// exactly one stream carrying the sample rate the engine was given.
TEST_F(BraveOnDeviceSpeechRecognitionEngineTest, StartsWhenSessionArrivesLast) {
  FakeAsrSession session;
  CreateEngine(&session);
  SetAudioParameters();
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(1, session.start_count());
  EXPECT_EQ(static_cast<uint32_t>(kSampleRateHz), session.sample_rate_hz());
}

TEST_F(BraveOnDeviceSpeechRecognitionEngineTest,
       StartsWhenAudioParametersArriveLast) {
  FakeAsrSession session;
  CreateEngine(&session);
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(0, session.start_count());

  SetAudioParameters();
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(1, session.start_count());
  EXPECT_EQ(static_cast<uint32_t>(kSampleRateHz), session.sample_rate_hz());
}

// Tests that a second set of audio parameters does not start a second stream.
TEST_F(BraveOnDeviceSpeechRecognitionEngineTest, StartsOnlyOnce) {
  FakeAsrSession session;
  CreateEngine(&session);
  SetAudioParameters();
  base::RunLoop().RunUntilIdle();
  SetAudioParameters();
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(1, session.start_count());
}

// Tests that setting audio parameters does not reach the base class's UI
// thread Core, which is not built for these sessions. Without the override
// this DCHECKs on a null SequenceBound.
TEST_F(BraveOnDeviceSpeechRecognitionEngineTest,
       SetAudioParametersSkipsTheCore) {
  FakeAsrSession session;
  CreateEngine(&session);

  SetAudioParameters();
  base::RunLoop().RunUntilIdle();
}

// Tests that a refused session is silent rather than reported. The recognizer
// ignores engine errors until it starts recording, and AudioChunksEnded ends a
// session that never got a worker.
TEST_F(BraveOnDeviceSpeechRecognitionEngineTest, NoSessionIsNotReported) {
  CreateEngine(nullptr);
  SetAudioParameters();
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(0, delegate_.results_count());
  EXPECT_EQ(0, delegate_.error_count());
}

// Tests that with no worker to wait on, the end of audio ends recognition the
// way upstream does, by reporting an empty result set.
TEST_F(BraveOnDeviceSpeechRecognitionEngineTest,
       AudioChunksEndedWithoutStreamReportsEmptyResults) {
  CreateEngine(nullptr);
  SetAudioParameters();
  base::RunLoop().RunUntilIdle();

  engine_->AudioChunksEnded();

  EXPECT_EQ(1, delegate_.results_count());
  EXPECT_TRUE(delegate_.last_results_empty());
}

// Tests that the end of audio closes the input stream and reports nothing.
// Closing it is what makes the worker emit its final result, and reporting an
// empty result set here would end recognition before that arrived.
TEST_F(BraveOnDeviceSpeechRecognitionEngineTest,
       AudioChunksEndedClosesStreamAndKeepsResponder) {
  FakeAsrSession session;
  CreateEngine(&session);
  SetAudioParameters();
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(1, session.start_count());

  base::RunLoop stream_closed;
  session.set_stream_disconnect_handler(stream_closed.QuitClosure());
  engine_->AudioChunksEnded();
  stream_closed.Run();

  // The final result still has somewhere to arrive on.
  EXPECT_TRUE(session.responder_connected());
  EXPECT_EQ(0, delegate_.results_count());
}

// Tests that ending recognition drops the session remote, which is the
// controller's lease on the worker and so the only thing telling it that this
// session is over.
TEST_F(BraveOnDeviceSpeechRecognitionEngineTest, EndRecognitionDropsSession) {
  FakeAsrSession session;
  CreateEngine(&session);
  SetAudioParameters();
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(1, session.start_count());

  base::RunLoop session_closed;
  session.set_session_disconnect_handler(session_closed.QuitClosure());
  engine_->EndRecognition();
  session_closed.Run();
}

}  // namespace content
