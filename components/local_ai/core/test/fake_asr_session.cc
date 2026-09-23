/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/test/fake_asr_session.h"

#include <utility>
#include <vector>

namespace local_ai {

FakeAsrSession::FakeAsrSession() = default;
FakeAsrSession::~FakeAsrSession() = default;

mojo::PendingRemote<mojom::AsrSession> FakeAsrSession::BindRemote() {
  return session_receiver.BindNewPipeAndPassRemote();
}

void FakeAsrSession::Start(
    on_device_model::mojom::AsrStreamOptionsPtr start_options,
    mojo::PendingReceiver<on_device_model::mojom::AsrStreamInput>
        pending_stream,
    mojo::PendingRemote<on_device_model::mojom::AsrStreamResponder>
        pending_responder) {
  options = std::move(start_options);
  stream_receiver.Bind(std::move(pending_stream));
  responder.Bind(std::move(pending_responder));
  started.SetValue();
}

void FakeAsrSession::AddAudioChunk(on_device_model::mojom::AudioDataPtr data) {
  audio_chunk.SetValue(std::move(data));
}

void FakeAsrSession::SendResult(const std::string& transcript, bool is_final) {
  std::vector<on_device_model::mojom::SpeechRecognitionResultPtr> results;
  results.push_back(on_device_model::mojom::SpeechRecognitionResult::New(
      transcript, is_final));
  responder->OnResponse(std::move(results));
  responder.FlushForTesting();
}

void FakeAsrSession::SendEmptyResult() {
  responder->OnResponse(
      std::vector<on_device_model::mojom::SpeechRecognitionResultPtr>());
  responder.FlushForTesting();
}

}  // namespace local_ai
