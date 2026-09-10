// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/location.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/on_device_speech_recognition.mojom.h"
#include "components/speech/audio_buffer.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/speech_recognition_session_config.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_utils.h"
#include "media/base/audio_parameters.h"
#include "media/base/channel_layout.h"
#include "media/mojo/mojom/speech_recognition_error.mojom.h"
#include "media/mojo/mojom/speech_recognition_error_code.mojom.h"
#include "media/mojo/mojom/speech_recognition_result.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/on_device_model/public/mojom/on_device_model.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// These headers are private to //content/browser (the Brave one via
// brave_content_browser_sources) and can only be reached from content's own
// test targets, which brave_unit_tests is not, so we workaround it with
// `nogncheck`.
#include "brave/content/browser/speech/brave_on_device_speech_recognition_engine.h"  // nogncheck
#include "content/browser/speech/speech_recognition_engine.h"  // nogncheck

namespace content {

// Defined in brave/content/browser/speech/
// brave_speech_recognition_manager_impl.cc. Declared here rather than included
// because they have no header: the chromium_src override that calls them
// declares them the same way, so that override depends on no Brave target.
bool UsesBraveOnDeviceSpeechEngine();
std::unique_ptr<SpeechRecognitionEngine> MakeOnDeviceSpeechEngine(
    const SpeechRecognitionSessionConfig& config);

namespace {

constexpr int kSampleRateHz = 16000;

// The worker end of one recognition. Members are public so tests can drive and
// inspect the pipes without an accessor for each one.
class FakeAsrSession : public local_ai::mojom::AsrSession,
                       public on_device_model::mojom::AsrStreamInput {
 public:
  mojo::PendingRemote<local_ai::mojom::AsrSession> BindRemote() {
    return session_receiver.BindNewPipeAndPassRemote();
  }

  // local_ai::mojom::AsrSession:
  void Start(on_device_model::mojom::AsrStreamOptionsPtr start_options,
             mojo::PendingReceiver<on_device_model::mojom::AsrStreamInput>
                 pending_stream,
             mojo::PendingRemote<on_device_model::mojom::AsrStreamResponder>
                 pending_responder) override {
    options = std::move(start_options);
    stream_receiver.Bind(std::move(pending_stream));
    responder.Bind(std::move(pending_responder));
    started.SetValue();
  }

  // on_device_model::mojom::AsrStreamInput:
  void AddAudioChunk(on_device_model::mojom::AudioDataPtr data) override {
    audio_chunk.SetValue(std::move(data));
  }

  void SendResult(const std::string& transcript, bool is_final) {
    std::vector<on_device_model::mojom::SpeechRecognitionResultPtr> results;
    results.push_back(on_device_model::mojom::SpeechRecognitionResult::New(
        transcript, is_final));
    responder->OnResponse(std::move(results));
    responder.FlushForTesting();
  }

  // Reports that nothing was recognized, then waits for delivery. This result
  // is what ends a session with no transcript, so it must survive filtering.
  void SendEmptyResult() {
    responder->OnResponse(
        std::vector<on_device_model::mojom::SpeechRecognitionResultPtr>());
    responder.FlushForTesting();
  }

  base::test::TestFuture<void> started;
  base::test::TestFuture<on_device_model::mojom::AudioDataPtr> audio_chunk;
  on_device_model::mojom::AsrStreamOptionsPtr options;
  mojo::Receiver<local_ai::mojom::AsrSession> session_receiver{this};
  mojo::Receiver<on_device_model::mojom::AsrStreamInput> stream_receiver{this};
  mojo::Remote<on_device_model::mojom::AsrStreamResponder> responder;
};

// Stands in for BraveContentBrowserClient, which always hands out a session.
class FakeContentBrowserClient : public ContentBrowserClient {
 public:
  explicit FakeContentBrowserClient(FakeAsrSession& session)
      : session_(&session) {}

  mojo::PendingRemote<local_ai::mojom::AsrSession> GetAsrSession() override {
    requested.SetValue();
    return session_->BindRemote();
  }

  // Signalled once the engine's UI thread hop reaches the embedder.
  base::test::TestFuture<void> requested;

 private:
  raw_ptr<FakeAsrSession> session_;
};

// SpeechRecognizerImpl is the real delegate. Strict, so an unexpected result or
// error fails the test rather than passing silently.
class MockDelegate : public SpeechRecognitionEngine::Delegate {
 public:
  MOCK_METHOD(void,
              OnSpeechRecognitionEngineResults,
              (const std::vector<media::mojom::WebSpeechRecognitionResultPtr>&),
              (override));
  MOCK_METHOD(void, OnSpeechRecognitionEngineEndOfUtterance, (), (override));
  MOCK_METHOD(void,
              OnSpeechRecognitionEngineError,
              (const media::mojom::SpeechRecognitionError&),
              (override));
};

// Matches the one result the engine reports per worker response.
MATCHER_P2(SingleResult, transcript, is_provisional, "") {
  return arg.size() == 1u && arg[0]->is_provisional == is_provisional &&
         arg[0]->hypotheses.size() == 1u &&
         arg[0]->hypotheses[0]->utterance == base::ASCIIToUTF16(transcript);
}

}  // namespace

class BraveOnDeviceSpeechRecognitionEngineTest : public testing::Test {
 protected:
  // Builds the engine against a client that hands out `session`. The session is
  // asked for on the UI thread, so it has not arrived when this returns.
  void CreateEngine(FakeAsrSession& session,
                    const std::string& language = "en-US",
                    bool interim_results = false) {
    client_ = std::make_unique<FakeContentBrowserClient>(session);
    client_setting_ =
        std::make_unique<ScopedContentBrowserClientSetting>(client_.get());
    SpeechRecognitionSessionConfig config;
    config.language = language;
    config.interim_results = interim_results;
    engine_ = std::make_unique<BraveOnDeviceSpeechRecognitionEngine>(config);
    engine_->set_delegate(&delegate_);
  }

  void SetAudioParameters() {
    engine_->SetAudioParameters(
        media::AudioParameters(media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
                               media::ChannelLayoutConfig::Mono(),
                               kSampleRateHz, kSampleRateHz / 100));
  }

  [[nodiscard]] bool WaitUntilSessionBound() {
    return base::test::RunUntil(
        [&] { return engine_->asr_session_.is_bound(); });
  }

  void FlushSessionRemote() { engine_->asr_session_.FlushForTesting(); }

  // Set before Start is queued, so unlike the fake's view it cannot be fooled
  // by a message that has been sent but not yet delivered.
  bool session_created() const { return engine_->session_created_; }

  BrowserTaskEnvironment task_environment_;
  testing::StrictMock<MockDelegate> delegate_;
  std::unique_ptr<FakeContentBrowserClient> client_;
  std::unique_ptr<ScopedContentBrowserClientSetting> client_setting_;
  std::unique_ptr<BraveOnDeviceSpeechRecognitionEngine> engine_;
};

// The sequence a real session runs, for a page that asked for interim
// results. Audio reaches the worker, interim results come back, the end of
// audio closes the input stream so the worker can emit its final result, and
// only then does ending recognition drop the session remote, which is what
// releases the worker.
TEST_F(BraveOnDeviceSpeechRecognitionEngineTest, NormalFlow) {
  FakeAsrSession session;
  CreateEngine(session, "en-US", /*interim_results=*/true);
  SetAudioParameters();
  ASSERT_TRUE(session.started.Wait());
  EXPECT_EQ(kSampleRateHz, static_cast<int>(session.options->sample_rate_hz));
  EXPECT_EQ("en-US", session.options->language);

  // Only verifies audio reaches the worker over the pipe TryCreateSession
  // created. Accumulating and converting it is the base class's, and tested
  // there.
  constexpr std::array<int16_t, 4> kSamples = {0, 16384, -16384, 32767};
  engine_->TakeAudioChunk(*base::MakeRefCounted<AudioChunk>(
      base::as_byte_span(kSamples), sizeof(int16_t)));
  EXPECT_TRUE(session.audio_chunk.Wait());

  EXPECT_CALL(delegate_, OnSpeechRecognitionEngineResults(
                             SingleResult("partial", /*is_provisional=*/true)));
  session.SendResult("partial", /*is_final=*/false);

  // Upstream would report an empty result here, which ends recognition before
  // the final result arrives. Brave closes the input stream instead, which is
  // what makes the worker emit that result, and leaves the responder bound.
  base::test::TestFuture<void> stream_closed;
  session.stream_receiver.set_disconnect_handler(stream_closed.GetCallback());
  engine_->AudioChunksEnded();
  ASSERT_TRUE(stream_closed.Wait());
  ASSERT_TRUE(session.responder.is_connected());

  EXPECT_CALL(delegate_, OnSpeechRecognitionEngineResults(
                             SingleResult("final", /*is_provisional=*/false)));
  session.SendResult("final", /*is_final=*/true);

  base::test::TestFuture<void> session_closed;
  session.session_receiver.set_disconnect_handler(session_closed.GetCallback());
  engine_->EndRecognition();
  EXPECT_TRUE(session_closed.Wait());
}

// Nothing between the engine and blink consults interim_results, so honoring
// the page's choice is the engine's job.
TEST_F(BraveOnDeviceSpeechRecognitionEngineTest,
       InterimResultsDroppedWhenNotRequested) {
  FakeAsrSession session;
  CreateEngine(session, "en-US", /*interim_results=*/false);
  SetAudioParameters();
  ASSERT_TRUE(session.started.Wait());

  // SendResult waits for delivery, so the strict delegate proves it was
  // dropped rather than merely late.
  session.SendResult("partial", /*is_final=*/false);

  EXPECT_CALL(delegate_, OnSpeechRecognitionEngineResults(
                             SingleResult("final", /*is_provisional=*/false)));
  session.SendResult("final", /*is_final=*/true);
}

// Past the end of audio the recognizer takes any result for the final one, so
// a provisional is unwanted even from a session that asked for interims.
TEST_F(BraveOnDeviceSpeechRecognitionEngineTest,
       InterimResultsDroppedAfterAudioEnds) {
  FakeAsrSession session;
  CreateEngine(session, "en-US", /*interim_results=*/true);
  SetAudioParameters();
  ASSERT_TRUE(session.started.Wait());

  engine_->AudioChunksEnded();

  // Asked for, but no longer of any use, so the strict delegate rejects it.
  session.SendResult("partial", /*is_final=*/false);

  EXPECT_CALL(delegate_, OnSpeechRecognitionEngineResults(
                             SingleResult("final", /*is_provisional=*/false)));
  session.SendResult("final", /*is_final=*/true);
}

// An empty result is what ends a session with no transcript, so filtering
// interims must not swallow it.
TEST_F(BraveOnDeviceSpeechRecognitionEngineTest,
       EmptyResultSurvivesInterimFiltering) {
  FakeAsrSession session;
  CreateEngine(session, "en-US", /*interim_results=*/false);
  SetAudioParameters();
  ASSERT_TRUE(session.started.Wait());

  EXPECT_CALL(delegate_, OnSpeechRecognitionEngineResults(testing::IsEmpty()));
  session.SendEmptyResult();
}

// The stream needs both the session remote and the audio parameters, which
// arrive asynchronously. The next two cover each arrival order.
TEST_F(BraveOnDeviceSpeechRecognitionEngineTest, StartsWhenSessionArrivesLast) {
  FakeAsrSession session;
  CreateEngine(session);

  // The session request is still in flight, so the parameters cannot start it.
  SetAudioParameters();
  ASSERT_FALSE(session_created());

  EXPECT_TRUE(session.started.Wait());
}

TEST_F(BraveOnDeviceSpeechRecognitionEngineTest,
       StartsWhenAudioParametersArriveLast) {
  FakeAsrSession session;
  CreateEngine(session);

  // The session is in, so the parameters are what is missing.
  ASSERT_TRUE(WaitUntilSessionBound());
  ASSERT_FALSE(session_created());

  SetAudioParameters();

  EXPECT_TRUE(session.started.Wait());
}

TEST_F(BraveOnDeviceSpeechRecognitionEngineTest, EmptyLanguageIsNotForwarded) {
  FakeAsrSession session;
  CreateEngine(session, /*language=*/"");
  SetAudioParameters();

  ASSERT_TRUE(session.started.Wait());
  EXPECT_FALSE(session.options->language.has_value());
}

// A second set of audio parameters must not start a second stream.
TEST_F(BraveOnDeviceSpeechRecognitionEngineTest, StartsOnlyOnce) {
  FakeAsrSession session;
  CreateEngine(session);
  SetAudioParameters();
  ASSERT_TRUE(session.started.Wait());

  // Consume the first start so a second one would show up as a new value.
  session.started.Clear();

  SetAudioParameters();
  // Makes sure a second Start, if one was sent, has reached the fake.
  FlushSessionRemote();

  EXPECT_FALSE(session.started.IsReady());
}

// Audio can end before the session request comes back. With no stream to close,
// the engine ends recognition the way upstream does, with an empty result.
TEST_F(BraveOnDeviceSpeechRecognitionEngineTest,
       AudioChunksEndedBeforeSessionArrives) {
  FakeAsrSession session;
  CreateEngine(session);
  SetAudioParameters();

  EXPECT_CALL(delegate_, OnSpeechRecognitionEngineResults(testing::IsEmpty()));
  engine_->AudioChunksEnded();

  // The recognizer ends the session once it has that empty result.
  engine_->EndRecognition();
}

// A recognition can be aborted while the session request is still in flight.
// The engine header promises no delegate callbacks after EndRecognition, and
// the session remote is the worker's lease, so a session arriving late must be
// dropped rather than started.
TEST_F(BraveOnDeviceSpeechRecognitionEngineTest,
       EndRecognitionBeforeSessionArrives) {
  FakeAsrSession session;
  CreateEngine(session);
  SetAudioParameters();

  engine_->EndRecognition();

  ASSERT_TRUE(client_->requested.Wait());
  base::test::TestFuture<void> session_closed;
  session.session_receiver.set_disconnect_handler(session_closed.GetCallback());

  // Dropping the reply destroys the remote it carries, so the worker is
  // released rather than leased out to an engine that has already ended.
  EXPECT_TRUE(session_closed.Wait());
  EXPECT_FALSE(session.started.IsReady());
}

// A worker that dies mid session is reported as an error and releases the
// session. AudioChunksEnded relies on this being what unblocks a recognizer
// waiting for a final result that is never coming.
TEST_F(BraveOnDeviceSpeechRecognitionEngineTest, WorkerDeathReportsError) {
  FakeAsrSession session;
  CreateEngine(session);
  SetAudioParameters();
  ASSERT_TRUE(session.started.Wait());

  EXPECT_CALL(
      delegate_,
      OnSpeechRecognitionEngineError(testing::Field(
          &media::mojom::SpeechRecognitionError::code,
          media::mojom::SpeechRecognitionErrorCode::kServiceNotAllowed)));

  base::test::TestFuture<void> session_closed;
  session.session_receiver.set_disconnect_handler(session_closed.GetCallback());
  // The worker holds both ends, so its death breaks them together. Upstream
  // funnels either one into the same handler, and ending recognition cancels
  // the notification for the other, so only one error is reported.
  session.stream_receiver.reset();
  session.responder.reset();

  // The error path ends recognition, which drops the session remote.
  EXPECT_TRUE(session_closed.Wait());
}

// The manager builds the engine for an on-device session through
// MakeOnDeviceSpeechEngine, so this is where the feature decides whether that
// session is served by Brave or by upstream.
class OnDeviceSpeechEngineSelectionTest : public testing::Test {
 protected:
  OnDeviceSpeechEngineSelectionTest()
      : client_(session_), client_setting_(&client_) {}

  // Runs a task queued after the engine's constructor, so an engine that asks
  // the embedder for a session has asked by the time this returns.
  [[nodiscard]] bool WaitForConstructionTasks() {
    base::test::TestFuture<void> drained;
    GetUIThreadTaskRunner({})->PostTask(FROM_HERE, drained.GetCallback());
    return drained.Wait();
  }

  base::test::ScopedFeatureList features_;
  BrowserTaskEnvironment task_environment_;
  FakeAsrSession session_;
  FakeContentBrowserClient client_;
  ScopedContentBrowserClientSetting client_setting_;
};

TEST_F(OnDeviceSpeechEngineSelectionTest, BuildsBraveEngineWhenEnabled) {
  features_.InitAndEnableFeature(local_ai::kBraveOnDeviceSpeechRecognition);
  ASSERT_TRUE(UsesBraveOnDeviceSpeechEngine());

  SpeechRecognitionSessionConfig config;
  std::unique_ptr<SpeechRecognitionEngine> engine =
      MakeOnDeviceSpeechEngine(config);

  // Asking the embedder for a session is what only Brave's engine does.
  EXPECT_TRUE(client_.requested.Wait());
}

TEST_F(OnDeviceSpeechEngineSelectionTest, BuildsUpstreamEngineWhenDisabled) {
  ASSERT_FALSE(UsesBraveOnDeviceSpeechEngine());

  SpeechRecognitionSessionConfig config;
  std::unique_ptr<SpeechRecognitionEngine> engine =
      MakeOnDeviceSpeechEngine(config);

  // Upstream's engine goes to the model broker instead, so the embedder is
  // never asked, and by now it would have been.
  ASSERT_TRUE(WaitForConstructionTasks());
  EXPECT_FALSE(client_.requested.IsReady());
}

}  // namespace content
