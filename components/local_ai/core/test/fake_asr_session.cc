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
  return session_receiver_.BindNewPipeAndPassRemote();
}

void FakeAsrSession::Start(
    on_device_model::mojom::AsrStreamOptionsPtr start_options,
    mojo::PendingReceiver<on_device_model::mojom::AsrStreamInput>
        pending_stream,
    mojo::PendingRemote<on_device_model::mojom::AsrStreamResponder>
        pending_responder) {
  options_ = std::move(start_options);
  stream_receiver_.Bind(std::move(pending_stream));
  responder_.Bind(std::move(pending_responder));
  started_.SetValue();
}

void FakeAsrSession::AddAudioChunk(on_device_model::mojom::AudioDataPtr data) {
  if (!audio_chunk_.IsReady()) {
    audio_chunk_.SetValue(std::move(data));
  }

  if (pending_transcript_) {
    SendResultInternal(*std::exchange(pending_transcript_, std::nullopt),
                       /*is_final=*/true);
  }
}

void FakeAsrSession::SendResult(const std::string& transcript, bool is_final) {
  SendResultInternal(transcript, is_final);
  responder_.FlushForTesting();
}

void FakeAsrSession::SendEmptyResult() {
  responder_->OnResponse(
      std::vector<on_device_model::mojom::SpeechRecognitionResultPtr>());
  responder_.FlushForTesting();
}

void FakeAsrSession::RespondOnNextAudioChunk(const std::string& transcript) {
  pending_transcript_ = transcript;
}

void FakeAsrSession::SendResultInternal(const std::string& transcript,
                                        bool is_final) {
  std::vector<on_device_model::mojom::SpeechRecognitionResultPtr> results;
  results.push_back(on_device_model::mojom::SpeechRecognitionResult::New(
      transcript, is_final));
  responder_->OnResponse(std::move(results));
}

}  // namespace local_ai
