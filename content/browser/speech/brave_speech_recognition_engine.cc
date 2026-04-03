/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/content/browser/speech/brave_speech_recognition_engine.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/task/bind_post_task.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_client.h"

namespace content {

BraveSpeechRecognitionEngine::BraveSpeechRecognitionEngine(
    const SpeechRecognitionSessionConfig& config)
    : OnDeviceSpeechRecognitionEngine(config) {
  // Acquire the service remote on UI and posts the result back to IO.
  ui_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&BraveSpeechRecognitionEngine::GetServiceOnUI,
                     config_.initial_context.render_process_id,
                     config_.initial_context.render_frame_id,
                     base::BindPostTaskToCurrentDefault(base::BindOnce(
                         &BraveSpeechRecognitionEngine::OnServiceReady,
                         weak_factory_.GetWeakPtr()))));
}

BraveSpeechRecognitionEngine::~BraveSpeechRecognitionEngine() = default;

// static
void BraveSpeechRecognitionEngine::GetServiceOnUI(
    int render_process_id,
    int render_frame_id,
    base::OnceCallback<void(
        mojo::PendingRemote<local_ai::mojom::OnDeviceSpeechRecognitionService>)>
        callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  RenderFrameHost* rfh =
      RenderFrameHost::FromID(render_process_id, render_frame_id);
  if (!rfh) {
    std::move(callback).Run({});
    return;
  }
  std::move(callback).Run(
      GetContentClient()->browser()->GetOnDeviceSpeechRecognitionService(
          rfh->GetBrowserContext()));
}

void BraveSpeechRecognitionEngine::OnServiceReady(
    mojo::PendingRemote<local_ai::mojom::OnDeviceSpeechRecognitionService>
        pending) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  if (!pending.is_valid()) {
    return;
  }
  service_.Bind(std::move(pending));
  MaybeCreateSession();
}

void BraveSpeechRecognitionEngine::SetAudioParameters(
    media::AudioParameters audio_parameters) {
  SpeechRecognitionEngine::SetAudioParameters(audio_parameters);
  MaybeCreateSession();
}

void BraveSpeechRecognitionEngine::MaybeCreateSession() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  if (session_created_ || !service_.is_bound() ||
      !audio_parameters_.IsValid()) {
    return;
  }
  session_created_ = true;

  auto options = on_device_model::mojom::AsrStreamOptions::New();
  options->sample_rate_hz = audio_parameters_.sample_rate();

  // Create pipe pairs and bind on IO, same thread as
  // the engine. Mojo buffers messages until the worker
  // binds its ends.
  mojo::PendingRemote<on_device_model::mojom::AsrStreamInput> asr_stream;
  mojo::PendingReceiver<on_device_model::mojom::AsrStreamResponder>
      asr_stream_responder;

  service_->CreateSession(
      std::move(options), asr_stream.InitWithNewPipeAndPassReceiver(),
      asr_stream_responder.InitWithNewPipeAndPassRemote(), base::DoNothing());

  asr_stream_.Bind(std::move(asr_stream));
  asr_stream_responder_.Bind(std::move(asr_stream_responder));
  // Safe to use Unretained: the receiver is owned by
  // the base class and reset in EndRecognition, which
  // cancels the handler before |this| is destroyed.
  asr_stream_responder_.set_disconnect_handler(
      base::BindOnce(&BraveSpeechRecognitionEngine::OnResponderDisconnect,
                     base::Unretained(this)));
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
  // to ProcessFinalResult -> EndRecognition.
  // Base class does not do this.
  if (has_final) {
    delegate_->OnSpeechRecognitionEngineEndOfUtterance();
  }
}

void BraveSpeechRecognitionEngine::OnResponderDisconnect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  delegate_->OnSpeechRecognitionEngineError(
      media::mojom::SpeechRecognitionError(
          media::mojom::SpeechRecognitionErrorCode::kAborted,
          media::mojom::SpeechAudioErrorDetails::kNone));
}

}  // namespace content
