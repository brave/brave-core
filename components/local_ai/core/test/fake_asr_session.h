/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_TEST_FAKE_ASR_SESSION_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_TEST_FAKE_ASR_SESSION_H_

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
// `OnDeviceSpeechRecognitionController` hands out. Members are public so tests
// can drive and inspect the pipes without an accessor for each one.
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

  base::test::TestFuture<void> started;
  on_device_model::mojom::AsrStreamOptionsPtr options;
  base::test::TestFuture<on_device_model::mojom::AudioDataPtr> audio_chunk;

  mojo::Receiver<mojom::AsrSession> session_receiver{this};
  mojo::Receiver<on_device_model::mojom::AsrStreamInput> stream_receiver{this};
  mojo::Remote<on_device_model::mojom::AsrStreamResponder> responder;
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_TEST_FAKE_ASR_SESSION_H_
