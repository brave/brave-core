// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/content/browser/speech/brave_on_device_speech_recognition_engine.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/task/bind_post_task.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_client.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "services/on_device_model/public/mojom/on_device_model.mojom.h"

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
              brave_weak_factory_.GetWeakPtr()))));
}

BraveOnDeviceSpeechRecognitionEngine::~BraveOnDeviceSpeechRecognitionEngine() =
    default;

void BraveOnDeviceSpeechRecognitionEngine::OnAsrSessionReady(
    mojo::PendingRemote<local_ai::mojom::AsrSession> pending) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  if (!pending.is_valid()) {
    // No session to be had, because the frame is gone. Nothing is reported
    // from here: the recognizer ignores engine errors until it starts
    // recording, and AudioChunksEnded already ends a session that has no
    // worker. The base engine is equally silent when its own model client
    // fails.
    return;
  }
  asr_session_.Bind(std::move(pending));
  TryStartSession();
}

void BraveOnDeviceSpeechRecognitionEngine::SetAudioParameters(
    media::AudioParameters audio_parameters) {
  // Call the grandparent to set audio_parameters_ without reaching the base's
  // UI thread Core, which is not built for these sessions. Calling the base
  // here would AsyncCall a null SequenceBound and DCHECK.
  SpeechRecognitionEngine::SetAudioParameters(audio_parameters);
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

  // Build the pipes on IO, the engine's own sequence. Mojo buffers the
  // messages until the worker binds the far ends.
  mojo::PendingRemote<on_device_model::mojom::AsrStreamInput> asr_stream;
  mojo::PendingReceiver<on_device_model::mojom::AsrStreamResponder>
      asr_stream_responder;
  asr_session_->Start(std::move(options),
                      asr_stream.InitWithNewPipeAndPassReceiver(),
                      asr_stream_responder.InitWithNewPipeAndPassRemote());

  // Brave's controller hands back the same pair the base's Core does, so
  // let the base bind it. That keeps asr_stream_, asr_stream_responder_
  // and the responder disconnect handler in one place, and the reset in
  // the base's EndRecognition keeps matching the bind.
  OnAsrStreamCreated(std::move(asr_stream), std::move(asr_stream_responder));
}

void BraveOnDeviceSpeechRecognitionEngine::AudioChunksEnded() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  // Closing the input stream triggers our worker to emit a final result, so
  // the responder stays bound for that result to arrive on. Our delegate_,
  // SpeechRecognizerImpl, waits in STATE_WAITING_FINAL_RESULT until the
  // worker reports through OnResponse, then calls EndRecognition, where the
  // responder is dropped. Nothing bounds that wait. A worker that dies trips
  // the base's responder disconnect handler, which reports an error and ends
  // the session, but one that stays alive and never reports leaves the session
  // open until the page aborts it.
  if (asr_stream_.is_bound()) {
    asr_stream_.reset();
    return;
  }

  // No input stream to close, either because the worker died and the base's
  // responder disconnect handler already reset it, or because a session was
  // never started. No final result is coming and there is nothing to wait
  // for, so fall through to upstream which just sends an empty result to
  // trigger EndRecognition.
  OnDeviceSpeechRecognitionEngine::AudioChunksEnded();
}

void BraveOnDeviceSpeechRecognitionEngine::EndRecognition() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  OnDeviceSpeechRecognitionEngine::EndRecognition();
  // Drop the remote to tell the controller this session ended.
  asr_session_.reset();
}

}  // namespace content
