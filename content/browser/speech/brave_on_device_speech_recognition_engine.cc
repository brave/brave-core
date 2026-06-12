// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/content/browser/speech/brave_on_device_speech_recognition_engine.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/task/bind_post_task.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_client.h"

namespace content {

namespace {

// Runs on UI with no pointers to the engine. Acquires the AsrSession
// PendingRemote from Brave's controller and delivers it back to IO via
// `callback`.
void GetAsrSessionOnUI(
    GlobalRenderFrameHostId global_id,
    base::OnceCallback<void(mojo::PendingRemote<local_ai::mojom::AsrSession>)>
        callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  RenderFrameHost* rfh = RenderFrameHost::FromID(global_id);
  if (!rfh) {
    std::move(callback).Run({});
    return;
  }
  std::move(callback).Run(
      GetContentClient()->browser()->GetAsrSession(rfh->GetBrowserContext()));
}

}  // namespace

BraveOnDeviceSpeechRecognitionEngine::BraveOnDeviceSpeechRecognitionEngine(
    const SpeechRecognitionSessionConfig& config)
    : OnDeviceSpeechRecognitionEngine(config) {
  // Acquire the AsrSession remote on UI and post the result back to IO.
  GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(
          &GetAsrSessionOnUI, config_.initial_context.global_id,
          base::BindPostTaskToCurrentDefault(base::BindOnce(
              &BraveOnDeviceSpeechRecognitionEngine::OnAsrSessionReady,
              weak_factory_.GetWeakPtr()))));
}

BraveOnDeviceSpeechRecognitionEngine::~BraveOnDeviceSpeechRecognitionEngine() =
    default;

void BraveOnDeviceSpeechRecognitionEngine::OnAsrSessionReady(
    mojo::PendingRemote<local_ai::mojom::AsrSession> pending) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  if (!pending.is_valid()) {
    return;
  }
  asr_session_.Bind(std::move(pending));
  MaybeCreateSession();
}

int BraveOnDeviceSpeechRecognitionEngine::GetDesiredAudioChunkDurationMs()
    const {
  // The base engine flushes accumulated audio to the worker on every
  // TakeAudioChunk call, so the audio-chunk duration is the streaming
  // cadence. Request 500 ms chunks to match the WASM worker's own
  // 500 ms partial-result minimum and to avoid the per-100 ms decode
  // passes the base default (kAudioPacketIntervalMs) would otherwise
  // drive.
  constexpr int kBraveAudioPacketIntervalMs = 500;
  return kBraveAudioPacketIntervalMs;
}

void BraveOnDeviceSpeechRecognitionEngine::SetAudioParameters(
    media::AudioParameters audio_parameters) {
  // Call the grandparent to set audio_parameters_ without forwarding
  // the sample rate to the base's UI-thread Core, whose ModelBroker
  // session path Brave does not use.
  SpeechRecognitionEngine::SetAudioParameters(audio_parameters);
  MaybeCreateSession();
}

void BraveOnDeviceSpeechRecognitionEngine::MaybeCreateSession() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  if (session_created_ || !asr_session_.is_bound() ||
      !audio_parameters_.IsValid()) {
    return;
  }
  session_created_ = true;

  auto options = on_device_model::mojom::AsrStreamOptions::New();
  options->sample_rate_hz = audio_parameters_.sample_rate();

  // Build the pipes on IO (same sequence as the engine). Mojo will
  // buffer messages until the worker binds the far ends.
  mojo::PendingRemote<on_device_model::mojom::AsrStreamInput> asr_stream;
  mojo::PendingReceiver<on_device_model::mojom::AsrStreamResponder>
      asr_stream_responder;

  asr_session_->Start(std::move(options),
                      asr_stream.InitWithNewPipeAndPassReceiver(),
                      asr_stream_responder.InitWithNewPipeAndPassRemote());

  asr_stream_.Bind(std::move(asr_stream));
  asr_stream_responder_.Bind(std::move(asr_stream_responder));
  // Safe to use Unretained: asr_stream_responder_ is reset in
  // EndRecognition, which cancels the handler before `this` is
  // destroyed.
  asr_stream_responder_.set_disconnect_handler(base::BindOnce(
      &BraveOnDeviceSpeechRecognitionEngine::OnResponderDisconnect,
      base::Unretained(this)));
}

void BraveOnDeviceSpeechRecognitionEngine::AudioChunksEnded() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  // Flush any buffered audio that didn't reach the 2 s threshold.
  if (!accumulated_audio_data_.empty() && asr_stream_.is_bound()) {
    asr_stream_->AddAudioChunk(ConvertAccumulatedAudioData());
  }
  // Disconnect the stream so the worker flushes remaining audio and
  // emits the final result via OnResponse. The FSM waits in
  // STATE_WAITING_FINAL_RESULT until we deliver it, then calls
  // EndRecognition.
  asr_stream_.reset();
}

void BraveOnDeviceSpeechRecognitionEngine::EndRecognition() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  OnDeviceSpeechRecognitionEngine::EndRecognition();
  asr_stream_responder_.reset();
  // Dropping the AsrSession remote ends the session. The controller decrements
  // off the receiver disconnect and arms its idle timer.
  asr_session_.reset();
}

void BraveOnDeviceSpeechRecognitionEngine::OnResponderDisconnect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  delegate_->OnSpeechRecognitionEngineError(
      media::mojom::SpeechRecognitionError(
          media::mojom::SpeechRecognitionErrorCode::kAborted,
          media::mojom::SpeechAudioErrorDetails::kNone));
}

}  // namespace content
