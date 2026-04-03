/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/content/browser/speech/brave_speech_recognition_engine.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/bind_post_task.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_client.h"

namespace content {

BraveSpeechRecognitionEngine::BraveSpeechRecognitionEngine(
    const SpeechRecognitionSessionConfig& config)
    : OnDeviceSpeechRecognitionEngine(config),
      service_(nullptr, base::OnTaskRunnerDeleter(nullptr)) {}

BraveSpeechRecognitionEngine::~BraveSpeechRecognitionEngine() = default;

void BraveSpeechRecognitionEngine::SetAudioParameters(
    media::AudioParameters audio_parameters) {
  // Store params in base class.
  SpeechRecognitionEngine::SetAudioParameters(audio_parameters);
  // Trigger our own session creation instead of the
  // base class's ModelBroker-based flow.
  ui_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&BraveSpeechRecognitionEngine::CreateSessionOnUI,
                     weak_factory_.GetWeakPtr()));
}

void BraveSpeechRecognitionEngine::AudioChunksEnded() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  // Flush any buffered audio that didn't reach the
  // threshold.
  if (!accumulated_audio_data_.empty() && asr_stream_.is_bound()) {
    asr_stream_->AddAudioChunk(ConvertAccumulatedAudioData());
  }
  // Disconnect stream. The worker's disconnect handler
  // will flush remaining audio and send the final result
  // via OnResponse. The FSM waits in
  // STATE_WAITING_FINAL_RESULT until we deliver it,
  // then calls EndRecognition.
  asr_stream_.reset();
}

void BraveSpeechRecognitionEngine::EndRecognition() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  // The FSM guarantees EndRecognition is only called
  // AFTER the final result is delivered (via OnResponse
  // → delegate → FSM EVENT_ENGINE_RESULT →
  // ProcessFinalResult). Safe to clean up everything.
  OnDeviceSpeechRecognitionEngine::EndRecognition();
  service_.reset();
}

void BraveSpeechRecognitionEngine::OnResponse(
    std::vector<on_device_model::mojom::SpeechRecognitionResultPtr> result) {
  bool has_final = false;
  for (const auto& r : result) {
    if (r->is_final) {
      has_final = true;
      break;
    }
  }

  // Base class converts results and calls delegate.
  OnDeviceSpeechRecognitionEngine::OnResponse(std::move(result));

  // Signal end of utterance so the FSM transitions
  // to ProcessFinalResult → EndRecognition.
  // Base class does not do this.
  if (has_final) {
    delegate_->OnSpeechRecognitionEngineEndOfUtterance();
  }
}

void BraveSpeechRecognitionEngine::CreateSessionOnUI() {
  if (session_created_ || !audio_parameters_.IsValid()) {
    return;
  }

  RenderFrameHost* rfh =
      RenderFrameHost::FromID(config_.initial_context.render_process_id,
                              config_.initial_context.render_frame_id);
  if (!rfh) {
    return;
  }

  auto pending =
      GetContentClient()->browser()->GetOnDeviceSpeechRecognitionService(
          rfh->GetBrowserContext());
  if (!pending.is_valid()) {
    return;
  }

  session_created_ = true;
  service_ = {
      new mojo::Remote<local_ai::mojom::OnDeviceSpeechRecognitionService>(
          std::move(pending)),
      base::OnTaskRunnerDeleter(ui_task_runner_)};

  auto options = on_device_model::mojom::AsrStreamOptions::New();
  options->sample_rate_hz = audio_parameters_.sample_rate();

  mojo::PendingRemote<on_device_model::mojom::AsrStreamInput> stream_remote;
  auto stream_receiver = stream_remote.InitWithNewPipeAndPassReceiver();

  // Store for OnSessionCreated to post to IO.
  pending_stream_ = std::move(stream_remote);

  // Disconnect handler: fires on UI if the worker drops
  // the responder. Posts to IO via WeakPtr so it's safe
  // even if the engine is destroyed before the handler
  // runs. Same pattern as SodaSpeechRecognitionEngineImpl.
  asr_stream_responder_.set_disconnect_handler(base::BindPostTask(
      io_task_runner_,
      base::BindOnce(&BraveSpeechRecognitionEngine::OnResponderDisconnect,
                     weak_factory_.GetWeakPtr())));

  (*service_)->CreateSession(
      std::move(options), std::move(stream_receiver),
      asr_stream_responder_.BindNewPipeAndPassRemote(),
      base::BindOnce(&BraveSpeechRecognitionEngine::OnSessionCreated,
                     weak_factory_.GetWeakPtr()));
}

void BraveSpeechRecognitionEngine::OnSessionCreated() {
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&BraveSpeechRecognitionEngine::OnAsrStreamCreated,
                     weak_factory_.GetWeakPtr(), std::move(pending_stream_)));
}

void BraveSpeechRecognitionEngine::OnAsrStreamCreated(
    mojo::PendingRemote<on_device_model::mojom::AsrStreamInput> remote) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  asr_stream_.Bind(std::move(remote));
}

void BraveSpeechRecognitionEngine::OnResponderDisconnect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  delegate_->OnSpeechRecognitionEngineError(
      media::mojom::SpeechRecognitionError(
          media::mojom::SpeechRecognitionErrorCode::kAborted,
          media::mojom::SpeechAudioErrorDetails::kNone));
}

}  // namespace content
