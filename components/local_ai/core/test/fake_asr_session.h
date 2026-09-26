/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_TEST_FAKE_ASR_SESSION_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_TEST_FAKE_ASR_SESSION_H_

#include <optional>
#include <string>

#include "base/test/test_future.h"
#include "brave/components/local_ai/core/on_device_speech_recognition.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/on_device_model/public/mojom/on_device_model.mojom.h"

namespace local_ai {

// The worker end of one recognition, in place of the session
// `OnDeviceSpeechRecognitionController` hands out.
class FakeAsrSession : public mojom::AsrSession,
                       public on_device_model::mojom::AsrStreamInput {
 public:
  FakeAsrSession();
  FakeAsrSession(const FakeAsrSession&) = delete;
  FakeAsrSession& operator=(const FakeAsrSession&) = delete;
  ~FakeAsrSession() override;

  mojo::PendingRemote<mojom::AsrSession> BindRemote();

  // mojom::AsrSession:
  void Start(on_device_model::mojom::AsrStreamOptionsPtr start_options,
             mojo::PendingReceiver<on_device_model::mojom::AsrStreamInput>
                 pending_stream,
             mojo::PendingRemote<on_device_model::mojom::AsrStreamResponder>
                 pending_responder) override;

  // on_device_model::mojom::AsrStreamInput:
  void AddAudioChunk(on_device_model::mojom::AudioDataPtr data) override;

  // Reports one result to the engine, then waits for it to be delivered so
  // that whatever the engine does with it has happened on return.
  void SendResult(const std::string& transcript, bool is_final);

  // Reports that nothing was recognized, then waits for delivery. This result
  // is what ends a session with no transcript, so it must survive filtering.
  void SendEmptyResult();

  // Reports `transcript` as a final result when the next audio chunk arrives,
  // once. Lets a test that drives a real recognition get an answer without
  // watching for the chunk itself.
  void RespondOnNextAudioChunk(const std::string& transcript);

  base::test::TestFuture<void>& started() { return started_; }

  const on_device_model::mojom::AsrStreamOptionsPtr& options() const {
    return options_;
  }

  // The first chunk the engine forwards. A recognition driven from a page
  // streams chunks for as long as it runs, and setting a future that already
  // holds a value fails the test, so later chunks are not kept.
  base::test::TestFuture<on_device_model::mojom::AudioDataPtr>& audio_chunk() {
    return audio_chunk_;
  }

  mojo::Receiver<mojom::AsrSession>& session_receiver() {
    return session_receiver_;
  }

  mojo::Receiver<on_device_model::mojom::AsrStreamInput>& stream_receiver() {
    return stream_receiver_;
  }

  mojo::Remote<on_device_model::mojom::AsrStreamResponder>& responder() {
    return responder_;
  }

 private:
  void SendResultInternal(const std::string& transcript, bool is_final);

  base::test::TestFuture<void> started_;
  on_device_model::mojom::AsrStreamOptionsPtr options_;
  base::test::TestFuture<on_device_model::mojom::AudioDataPtr> audio_chunk_;
  mojo::Receiver<mojom::AsrSession> session_receiver_{this};
  mojo::Receiver<on_device_model::mojom::AsrStreamInput> stream_receiver_{this};
  mojo::Remote<on_device_model::mojom::AsrStreamResponder> responder_;
  std::optional<std::string> pending_transcript_;
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_TEST_FAKE_ASR_SESSION_H_
