// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/content/browser/speech/brave_on_device_speech_recognition_engine.h"

#include <utility>
#include <vector>

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
  // Starts the stream if the session remote has already arrived.
  TryCreateSession();
}

void BraveOnDeviceSpeechRecognitionEngine::AudioChunksEnded() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  audio_ended_ = true;
  // Closing the input stream makes the worker emit its final result, so the
  // responder stays bound for it. Upstream would end recognition with an empty
  // result before that arrives, so we have to override this behavior to reply
  // with a final result from the worker instead.
  if (asr_stream_.is_bound()) {
    asr_stream_.reset();
    // Nothing else reports a worker that stays alive but never answers. The
    // timer is a member, so it cannot fire after `this` is destroyed.
    final_result_timer_.Start(
        FROM_HERE, kFinalResultTimeout,
        base::BindOnce(
            &BraveOnDeviceSpeechRecognitionEngine::OnFinalResultTimeout,
            base::Unretained(this)));
    return;
  }

  // No stream, so no final result is coming.
  OnDeviceSpeechRecognitionEngine::AudioChunksEnded();
}

void BraveOnDeviceSpeechRecognitionEngine::EndRecognition() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  final_result_timer_.Stop();
  OnDeviceSpeechRecognitionEngine::EndRecognition();
  // Drop any GetAsrSession reply still in flight, so it cannot start a stream.
  brave_weak_factory_.InvalidateWeakPtrs();
  asr_session_.reset();
}

void BraveOnDeviceSpeechRecognitionEngine::OnResponse(
    std::vector<on_device_model::mojom::SpeechRecognitionResultPtr> result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  // Any message means the worker is still alive and working, reset the timer.
  if (final_result_timer_.IsRunning()) {
    final_result_timer_.Reset();
  }

  // Nothing between here and blink consults interim_results, so the engine is
  // the only place that can honor it. Past the end of audio a provisional is
  // unwanted regardless: the recognizer reports it and then keeps waiting.
  if (audio_ended_ || !config_.interim_results) {
    const bool had_results = !result.empty();
    std::erase_if(result, [](const auto& r) { return !r->is_final; });
    // An empty vector means nothing was recognized, and that result is what
    // ends the session, so it still has to go through.
    if (had_results && result.empty()) {
      return;
    }
  }

  OnDeviceSpeechRecognitionEngine::OnResponse(std::move(result));
}

void BraveOnDeviceSpeechRecognitionEngine::OnFinalResultTimeout() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  // The empty result makes the recognizer end the session, which releases the
  // worker.
  OnDeviceSpeechRecognitionEngine::AudioChunksEnded();
}

void BraveOnDeviceSpeechRecognitionEngine::OnAsrSessionReady(
    mojo::PendingRemote<local_ai::mojom::AsrSession> pending) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  if (!pending.is_valid()) {
    return;
  }
  asr_session_.Bind(std::move(pending));
  // Starts the stream if the audio parameters have already arrived.
  TryCreateSession();
}

void BraveOnDeviceSpeechRecognitionEngine::TryCreateSession() {
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
