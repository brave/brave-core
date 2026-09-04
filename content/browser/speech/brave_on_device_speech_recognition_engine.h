// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CONTENT_BROWSER_SPEECH_BRAVE_ON_DEVICE_SPEECH_RECOGNITION_ENGINE_H_
#define BRAVE_CONTENT_BROWSER_SPEECH_BRAVE_ON_DEVICE_SPEECH_RECOGNITION_ENGINE_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/local_ai/core/on_device_speech_recognition.mojom.h"
#include "content/browser/speech/on_device_speech_recognition_engine_impl.h"
#include "content/common/content_export.h"
#include "media/base/audio_parameters.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace content {

// Serves a Web Speech session from Brave's own speech worker, opening its
// recognition stream on a session from ContentBrowserClient::GetAsrSession
// rather than the ModelBroker session the base class builds.
//
// Lives on the IO thread, with every Mojo binding and WeakPtr.
class CONTENT_EXPORT BraveOnDeviceSpeechRecognitionEngine
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

 private:
  friend class BraveOnDeviceSpeechRecognitionEngineTest;

  void OnAsrSessionReady(
      mojo::PendingRemote<local_ai::mojom::AsrSession> pending);

  // Creates the worker stream once both the AsrSession remote and the audio
  // parameters are in. They arrive in either order, so this runs after each and
  // does nothing until it has both.
  void TryStartSession();

  // This recognition's session with the speech worker.
  mojo::Remote<local_ai::mojom::AsrSession> asr_session_;

  bool session_created_ = false;

  base::WeakPtrFactory<BraveOnDeviceSpeechRecognitionEngine>
      brave_weak_factory_{this};
};

}  // namespace content

#endif  // BRAVE_CONTENT_BROWSER_SPEECH_BRAVE_ON_DEVICE_SPEECH_RECOGNITION_ENGINE_H_
