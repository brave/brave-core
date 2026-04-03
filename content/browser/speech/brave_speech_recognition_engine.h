/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CONTENT_BROWSER_SPEECH_BRAVE_SPEECH_RECOGNITION_ENGINE_H_
#define BRAVE_CONTENT_BROWSER_SPEECH_BRAVE_SPEECH_RECOGNITION_ENGINE_H_

#include <vector>

#include "brave/components/local_ai/core/on_device_speech_recognition.mojom.h"
// This file must be compiled within the content library
// (via content/browser/sources.gni) because the base
// class header is content-internal.
#include "content/browser/speech/on_device_speech_recognition_engine_impl.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace content {

// Subclass of Chromium's OnDeviceSpeechRecognitionEngine
// that bypasses the ModelBroker/optimization_guide flow
// and connects directly to our WASM whisper worker via
// ContentBrowserClient::GetOnDeviceSpeechRecognitionService.
//
// Reuses the base engine's audio buffering, int16→float
// conversion, and IO/UI threading. Only overrides session
// creation and stop flow.
class BraveSpeechRecognitionEngine : public OnDeviceSpeechRecognitionEngine {
 public:
  explicit BraveSpeechRecognitionEngine(
      const SpeechRecognitionSessionConfig& config);
  ~BraveSpeechRecognitionEngine() override;

  // SpeechRecognitionEngine:
  void SetAudioParameters(media::AudioParameters audio_parameters) override;
  void AudioChunksEnded() override;
  void EndRecognition() override;

  // on_device_model::mojom::AsrStreamResponder:
  void OnResponse(
      std::vector<on_device_model::mojom::SpeechRecognitionResultPtr> result)
      override;

 private:
  void CreateSessionOnUI();
  void OnSessionCreated();
  void OnAsrStreamCreated(
      mojo::PendingRemote<on_device_model::mojom::AsrStreamInput> remote);
  void OnResponderDisconnect();

  // Bound on UI thread; OnTaskRunnerDeleter ensures
  // destruction on UI regardless of which thread the
  // engine is destroyed on.
  std::unique_ptr<
      mojo::Remote<local_ai::mojom::OnDeviceSpeechRecognitionService>,
      base::OnTaskRunnerDeleter>
      service_;
  mojo::PendingRemote<on_device_model::mojom::AsrStreamInput> pending_stream_;

  base::WeakPtrFactory<BraveSpeechRecognitionEngine> weak_factory_{this};
};

}  // namespace content

#endif  // BRAVE_CONTENT_BROWSER_SPEECH_BRAVE_SPEECH_RECOGNITION_ENGINE_H_
