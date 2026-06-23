/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_CONTROLLER_H_
#define BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_CONTROLLER_H_

#include <memory>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/local_ai/core/background_web_contents.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/on_device_model/public/mojom/on_device_model.mojom.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace speech {

// `base::NoDestructor` singleton that owns the BackgroundWebContents
// hosting the ORT-Web Nemotron WASM worker and drives whole-model loading
// against that worker.
//
// State machine:
//   Idle -> BwcStarting -> ModelLoading -> Ready
class OnDeviceSpeechRecognitionController
    : public local_ai::mojom::SpeechRecognitionFactoryHost,
      public local_ai::mojom::AsrSession,
      public local_ai::BackgroundWebContents::Delegate,
      public ProfileObserver {
 public:
  static OnDeviceSpeechRecognitionController* Get();

  OnDeviceSpeechRecognitionController(
      const OnDeviceSpeechRecognitionController&) = delete;
  OnDeviceSpeechRecognitionController& operator=(
      const OnDeviceSpeechRecognitionController&) = delete;

  // Returns a remote the engine holds for one recognition. Dropping it ends
  // the session.
  mojo::PendingRemote<local_ai::mojom::AsrSession> GetAsrSession();

  // Binds the FactoryHost receiver for the ORT worker WebUI.
  void BindFactoryHost(
      mojo::PendingReceiver<local_ai::mojom::SpeechRecognitionFactoryHost>
          receiver);

  // local_ai::mojom::SpeechRecognitionFactoryHost:
  void RegisterFactory(
      mojo::PendingRemote<local_ai::mojom::SpeechRecognitionFactory> factory)
      override;

  // local_ai::mojom::AsrSession:
  void Start(
      on_device_model::mojom::AsrStreamOptionsPtr options,
      mojo::PendingReceiver<on_device_model::mojom::AsrStreamInput> stream,
      mojo::PendingRemote<on_device_model::mojom::AsrStreamResponder> responder)
      override;

  // local_ai::BackgroundWebContents::Delegate:
  void OnBackgroundContentsDestroyed(
      local_ai::BackgroundWebContents::DestroyReason reason) override;

  // ProfileObserver:
  void OnProfileWillBeDestroyed(Profile* profile) override;

 private:
  friend base::NoDestructor<OnDeviceSpeechRecognitionController>;

  enum class State {
    kIdle,
    kBwcStarting,
    kModelLoading,
    kReady,
  };

  OnDeviceSpeechRecognitionController();
  ~OnDeviceSpeechRecognitionController() override;

  void StartWorker();
  void EnsureBackgroundContents();
  void OnGuestProfileCreated(Profile* guest_profile);

  // Read the Nemotron graphs/companion files and whole-model weights off a
  // blocking sequence, then build the worker's ORT sessions in Init.
  void LoadOrtModel();
  void OnOrtFilesRead(local_ai::mojom::OrtModelFilesPtr files);
  void OnOrtInitResult(bool success);

  void ForwardPendingSessions();
  void ForwardSession(
      on_device_model::mojom::AsrStreamOptionsPtr options,
      mojo::PendingReceiver<on_device_model::mojom::AsrStreamInput> stream,
      mojo::PendingRemote<on_device_model::mojom::AsrStreamResponder>
          responder);

  void StartIdleTimer();
  void OnIdleTimeout();

  void OnFactoryDisconnected();
  void OnAsrSessionDisconnected();
  void TearDown();

  State state_ = State::kIdle;

  // The guest profile's primary OTR profile, which hosts the worker's
  // BackgroundWebContents and is the profile we observe for destruction.
  raw_ptr<Profile> otr_profile_ = nullptr;
  base::ScopedObservation<Profile, ProfileObserver> profile_observation_{this};

  std::unique_ptr<local_ai::BackgroundWebContents> background_web_contents_;

  mojo::ReceiverSet<local_ai::mojom::SpeechRecognitionFactoryHost>
      factory_host_receivers_;
  mojo::ReceiverSet<local_ai::mojom::AsrSession> asr_session_receivers_;
  mojo::Remote<local_ai::mojom::SpeechRecognitionFactory> factory_;

  struct PendingSession {
    PendingSession();
    PendingSession(PendingSession&&);
    PendingSession& operator=(PendingSession&&);
    ~PendingSession();

    on_device_model::mojom::AsrStreamOptionsPtr options;
    mojo::PendingReceiver<on_device_model::mojom::AsrStreamInput> stream;
    mojo::PendingRemote<on_device_model::mojom::AsrStreamResponder> responder;
  };
  std::vector<PendingSession> pending_sessions_;

  base::OneShotTimer idle_timer_;
  static constexpr base::TimeDelta kIdleTimeout = base::Seconds(60);

  base::WeakPtrFactory<OnDeviceSpeechRecognitionController> weak_factory_{this};
};

}  // namespace speech

#endif  // BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_CONTROLLER_H_
