/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_CONTROLLER_H_
#define BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_CONTROLLER_H_

#include <memory>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/browser/local_ai/background_web_contents_factory.h"
#include "brave/components/local_ai/core/background_web_contents.h"
#include "brave/components/local_ai/core/on_device_speech_recognition.mojom.h"
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
// hosting the WASM worker and drives model loading against that worker.
//
// State machine:
//   Idle -> BwcStarting -> WorkerStarting -> ModelLoading -> Ready
class OnDeviceSpeechRecognitionController
    : public local_ai::mojom::SpeechRecognitionFactoryHost,
      public local_ai::BackgroundWebContents::Delegate,
      public ProfileObserver {
 public:
  // Creates the BackgroundWebContents that hosts the worker. Production binds
  // this to local_ai::CreateBackgroundWebContents; tests inject a fake so the
  // state machine can be driven without a renderer or profile. The delegate is
  // passed weakly to match the factory's contract.
  using CreateBackgroundWebContentsCallback = base::RepeatingCallback<void(
      base::WeakPtr<local_ai::BackgroundWebContents::Delegate> delegate,
      local_ai::BackgroundWebContentsCreatedCallback created)>;

  static OnDeviceSpeechRecognitionController* Get();

  // Builds a standalone instance with an injected BackgroundWebContents
  // factory, bypassing the Get() singleton. For tests only.
  static std::unique_ptr<OnDeviceSpeechRecognitionController> CreateForTesting(
      CreateBackgroundWebContentsCallback create_background_web_contents);

  OnDeviceSpeechRecognitionController(
      const OnDeviceSpeechRecognitionController&) = delete;
  OnDeviceSpeechRecognitionController& operator=(
      const OnDeviceSpeechRecognitionController&) = delete;

  // Public so the CreateForTesting() std::unique_ptr can destroy the instance.
  // The Get() singleton is a NoDestructor and is never destroyed.
  ~OnDeviceSpeechRecognitionController() override;

  // Binds the FactoryHost receiver for the ORT worker WebUI.
  void BindFactoryHost(
      mojo::PendingReceiver<local_ai::mojom::SpeechRecognitionFactoryHost>
          receiver);

  // local_ai::mojom::SpeechRecognitionFactoryHost:
  void RegisterFactory(
      mojo::PendingRemote<local_ai::mojom::SpeechRecognitionFactory> factory)
      override;

  // local_ai::BackgroundWebContents::Delegate:
  void OnBackgroundContentsDestroyed(
      local_ai::BackgroundWebContents::DestroyReason reason) override;

  // ProfileObserver:
  void OnProfileWillBeDestroyed(Profile* profile) override;

 private:
  friend base::NoDestructor<OnDeviceSpeechRecognitionController>;

  enum class State {
    // No live worker. A Start() kicks off the pipeline.
    kIdle,
    // Creating the BackgroundWebContents. OnBackgroundContentsCreated()
    // advances this once the worker environment exists.
    kBwcStarting,
    // The BackgroundWebContents exists and the worker page is booting.
    // RegisterFactory() advances this when the worker registers its factory.
    kWorkerStarting,
    // The factory is bound and the model is loading. OnOrtInitResult()
    // advances this.
    kModelLoading,
    // The worker is initialized. A Start() is forwarded to the worker
    // immediately instead of being queued.
    kReady,
  };

  explicit OnDeviceSpeechRecognitionController(
      CreateBackgroundWebContentsCallback create_background_web_contents);

  void StartWorker();
  void OnBackgroundContentsCreated(
      std::unique_ptr<local_ai::BackgroundWebContents> background_web_contents,
      Profile* otr_profile);
  void OnStartupTimeout();

  // Read the Nemotron graphs/companion files and whole-model weights off a
  // blocking sequence, then build the worker's ORT sessions in Init.
  void LoadOrtModel();
  void OnOrtFilesRead(local_ai::mojom::OrtModelFilesPtr files);
  void OnOrtInitResult(bool success);

  void StartIdleTimer();
  void OnIdleTimeout();

  void OnFactoryDisconnected();
  void TearDown();

  CreateBackgroundWebContentsCallback create_background_web_contents_;

  State state_ = State::kIdle;

  // The guest profile's primary OTR profile, which hosts the worker's
  // BackgroundWebContents and is the profile we observe for destruction.
  raw_ptr<Profile> otr_profile_ = nullptr;
  base::ScopedObservation<Profile, ProfileObserver> profile_observation_{this};

  std::unique_ptr<local_ai::BackgroundWebContents> background_web_contents_;

  mojo::ReceiverSet<local_ai::mojom::SpeechRecognitionFactoryHost>
      factory_host_receivers_;
  mojo::Remote<local_ai::mojom::SpeechRecognitionFactory> factory_;

  // Tears the worker down if it never registers its factory. Cancelled in
  // RegisterFactory(); crashes past that point are caught by factory_'s
  // disconnect handler.
  base::OneShotTimer startup_timer_;
  static constexpr base::TimeDelta kStartupTimeout = base::Seconds(30);

  base::OneShotTimer idle_timer_;
  static constexpr base::TimeDelta kIdleTimeout = base::Seconds(60);

  base::WeakPtrFactory<OnDeviceSpeechRecognitionController> weak_factory_{this};
};

}  // namespace speech

#endif  // BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_CONTROLLER_H_
