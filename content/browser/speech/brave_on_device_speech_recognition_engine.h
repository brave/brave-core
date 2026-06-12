// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CONTENT_BROWSER_SPEECH_BRAVE_ON_DEVICE_SPEECH_RECOGNITION_ENGINE_H_
#define BRAVE_CONTENT_BROWSER_SPEECH_BRAVE_ON_DEVICE_SPEECH_RECOGNITION_ENGINE_H_

#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/local_ai/core/on_device_speech_recognition.mojom.h"
// Compiled inside the content library (via content/browser/sources.gni)
// because the base class header is content-internal.
#include "content/browser/speech/on_device_speech_recognition_engine_impl.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/on_device_model/public/mojom/on_device_model.mojom.h"

namespace content {

// Subclass of Chromium's OnDeviceSpeechRecognitionEngine that bypasses
// the ModelBroker / optimization_guide flow and instead routes the
// session through Brave's controller + WASM worker via
// ContentBrowserClient::GetAsrSession.
//
// Reuses the base engine's audio accumulation, int16 -> float
// conversion, and IO/UI threading. Overrides session creation
// and the stop flow.
//
// Threading: the engine lives on IO. The AsrSession remote is acquired
// asynchronously on UI via a free function (no member pointers cross
// threads). All Mojo bindings and WeakPtrs stay on IO.
class BraveOnDeviceSpeechRecognitionEngine
    : public OnDeviceSpeechRecognitionEngine {
 public:
  explicit BraveOnDeviceSpeechRecognitionEngine(
      const SpeechRecognitionSessionConfig& config);
  BraveOnDeviceSpeechRecognitionEngine(
      const BraveOnDeviceSpeechRecognitionEngine&) = delete;
  BraveOnDeviceSpeechRecognitionEngine& operator=(
      const BraveOnDeviceSpeechRecognitionEngine&) = delete;
  ~BraveOnDeviceSpeechRecognitionEngine() override;

  // SpeechRecognitionEngine:
  void SetAudioParameters(media::AudioParameters audio_parameters) override;
  void AudioChunksEnded() override;
  void EndRecognition() override;
  int GetDesiredAudioChunkDurationMs() const override;

 private:
  void OnAsrSessionReady(
      mojo::PendingRemote<local_ai::mojom::AsrSession> pending);
  void MaybeCreateSession();
  void OnResponderDisconnect();

  // Bound on IO. Messages are dispatched to the browser-side
  // controller on UI via Mojo. Replies return to IO.
  mojo::Remote<local_ai::mojom::AsrSession> asr_session_;

  // Receives transcription results from the worker. Bound in
  // MaybeCreateSession on IO; reset in EndRecognition.
  mojo::Receiver<on_device_model::mojom::AsrStreamResponder>
      asr_stream_responder_{this};

  // Idempotency guard for MaybeCreateSession.
  bool session_created_ = false;

  base::WeakPtrFactory<BraveOnDeviceSpeechRecognitionEngine> weak_factory_{
      this};
};

}  // namespace content

#endif  // BRAVE_CONTENT_BROWSER_SPEECH_BRAVE_ON_DEVICE_SPEECH_RECOGNITION_ENGINE_H_
