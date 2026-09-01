// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/content/browser/speech/brave_on_device_speech_recognition_engine.h"

#include <utility>

#include "base/functional/bind.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "services/on_device_model/public/mojom/on_device_model.mojom.h"

namespace content {

BraveOnDeviceSpeechRecognitionEngine::BraveOnDeviceSpeechRecognitionEngine(
    const SpeechRecognitionSessionConfig& config)
    : OnDeviceSpeechRecognitionEngine(config) {
  // The embedder hands out sessions on the UI thread.
  GetUIThreadTaskRunner({})->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce([]() {
        return GetContentClient()->browser()->GetAsrSession();
      }),
      base::BindOnce(&BraveOnDeviceSpeechRecognitionEngine::OnAsrSessionReady,
                     brave_weak_factory_.GetWeakPtr()));
}

BraveOnDeviceSpeechRecognitionEngine::~BraveOnDeviceSpeechRecognitionEngine() =
    default;

void BraveOnDeviceSpeechRecognitionEngine::SetAudioParameters(
    media::AudioParameters audio_parameters) {
  // Call the grandparent, so the base class cannot pass the sample rate to its
  // Core and start an optimization guide session of its own.
  SpeechRecognitionEngine::SetAudioParameters(audio_parameters);
  TryStartSession();
}

void BraveOnDeviceSpeechRecognitionEngine::AudioChunksEnded() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  // Closing the input stream makes the worker emit its final result, so the
  // responder stays bound for it. Upstream would end recognition with an empty
  // result before that arrives, so we have to override this behavior to reply
  // with a final result from the worker instead.
  if (asr_stream_.is_bound()) {
    asr_stream_.reset();
    return;
  }

  // No stream, so no final result is coming.
  OnDeviceSpeechRecognitionEngine::AudioChunksEnded();
}

void BraveOnDeviceSpeechRecognitionEngine::EndRecognition() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  OnDeviceSpeechRecognitionEngine::EndRecognition();
  // Drop any GetAsrSession reply still in flight, so it cannot start a stream.
  brave_weak_factory_.InvalidateWeakPtrs();
  asr_session_.reset();
}

void BraveOnDeviceSpeechRecognitionEngine::OnAsrSessionReady(
    mojo::PendingRemote<local_ai::mojom::AsrSession> pending) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  asr_session_.Bind(std::move(pending));
  TryStartSession();
}

void BraveOnDeviceSpeechRecognitionEngine::TryStartSession() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  if (session_created_ || !asr_session_.is_bound() ||
      !audio_parameters_.IsValid()) {
    return;
  }
  session_created_ = true;

  auto options = on_device_model::mojom::AsrStreamOptions::New();
  options->sample_rate_hz = audio_parameters_.sample_rate();
  if (!config_.language.empty()) {
    options->language = config_.language;
  }

  mojo::PendingRemote<on_device_model::mojom::AsrStreamInput> asr_stream;
  mojo::PendingReceiver<on_device_model::mojom::AsrStreamResponder>
      asr_stream_responder;
  asr_session_->Start(std::move(options),
                      asr_stream.InitWithNewPipeAndPassReceiver(),
                      asr_stream_responder.InitWithNewPipeAndPassRemote());

  // The base class owns both bindings and resets them in EndRecognition.
  OnAsrStreamCreated(std::move(asr_stream), std::move(asr_stream_responder));
}

}  // namespace content
