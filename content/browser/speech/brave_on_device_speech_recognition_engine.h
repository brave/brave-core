// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CONTENT_BROWSER_SPEECH_BRAVE_ON_DEVICE_SPEECH_RECOGNITION_ENGINE_H_
#define BRAVE_CONTENT_BROWSER_SPEECH_BRAVE_ON_DEVICE_SPEECH_RECOGNITION_ENGINE_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/local_ai/core/on_device_speech_recognition.mojom.h"
// Compiled inside the content library (via content/browser/sources.gni)
// because the base class header is content-internal.
#include "content/browser/speech/on_device_speech_recognition_engine_impl.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace content {

// Subclass of Chromium's OnDeviceSpeechRecognitionEngine that serves the
// session from Brave's own model, reached through Brave's controller and
// WASM worker via ContentBrowserClient::GetAsrSession, instead of the
// ModelBroker session the base class would create.
//
// Everything else is inherited: audio accumulation, the int16 to float
// conversion, the result plumbing in OnResponse, the stream and responder
// bindings in OnAsrStreamCreated, the audio chunk cadence, and the IO and
// UI threading. Only the session source and the stop flow differ.
//
// Note that no UI thread Core is built for these sessions. Brave's
// substitution in on_device_speech_recognition_engine_impl.cc returns from the
// base constructor before creating it, because nothing here needs it and
// leaving it to build would ask the optimization guide broker for assets.
// EndRecognition's core_.Reset() is a no-op on the null SequenceBound that
// leaves behind. The SetAudioParameters override below is what keeps the base
// from calling AsyncCall on it, which would DCHECK.
//
// Threading: the engine lives on IO. The AsrSession remote is acquired
// asynchronously on UI via a free function so that no member pointers
// cross threads. All Mojo bindings and WeakPtrs stay on IO.
//
// Exported for the same reason the base class is: this is compiled into the
// content library, and the unit tests are not.
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
  void OnAsrSessionReady(
      mojo::PendingRemote<local_ai::mojom::AsrSession> pending);

  // Creates the worker stream once the AsrSession remote and the audio
  // parameters are both in. They arrive asynchronously and in either
  // order, so this runs after each one and does nothing until it has
  // both. Mirrors the base's Core::TryCreateSession, which rendezvouses
  // its own two inputs the same way.
  void TryStartSession();

  // Bound on IO. Carries one Start message per session and then stays
  // open as the controller's lease on the worker, so dropping it is what
  // tells the controller that this session ended.
  mojo::Remote<local_ai::mojom::AsrSession> asr_session_;

  // Idempotency guard for TryStartSession, matching the base's own
  // session_created_ in OnDeviceSpeechRecognitionEngine::Core.
  bool session_created_ = false;

  // Named apart from the base class's own weak_factory_ so the two are
  // not confused. This one hands out WeakPtrs to the subclass.
  base::WeakPtrFactory<BraveOnDeviceSpeechRecognitionEngine>
      brave_weak_factory_{this};
};

}  // namespace content

#endif  // BRAVE_CONTENT_BROWSER_SPEECH_BRAVE_ON_DEVICE_SPEECH_RECOGNITION_ENGINE_H_
