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
#include "services/on_device_model/public/mojom/on_device_model.mojom.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace speech {

// `base::NoDestructor` singleton that owns the BackgroundWebContents hosting
// the WASM worker and hands out per-recognition AsrSessions.
//
// State machine:
//   kIdle -> kBwcStarting -> kWorkerStarting -> kModelLoading -> kReady
//
// TearDown() returns to kIdle from any state on idle/startup timeout or
// profile/contents destruction; the next Start() restarts the pipeline. A
// Start() that arrives before kReady is queued in pending_sessions_ and
// forwarded once Ready.
class OnDeviceSpeechRecognitionController
    : public local_ai::mojom::AsrSession,
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

  // Returns a remote the engine holds for one recognition. Dropping it ends
  // the session.
  mojo::PendingRemote<local_ai::mojom::AsrSession> GetAsrSession();

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
    // No live worker. The first Start() calls StartWorker() and advances to
    // kBwcStarting.
    kIdle,
    // The BackgroundWebContents is being created asynchronously. Advances to
    // kWorkerStarting in OnBackgroundContentsCreated() once it exists.
    kBwcStarting,
    // The BackgroundWebContents exists and the worker page is booting.
    // Advances to kModelLoading in RegisterFactory() once the worker
    // registers its factory.
    kWorkerStarting,
    // The factory is bound and the model is loading. Advances to kReady in
    // OnOrtInitResult() on success.
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

  void StartIdleTimer();
  void OnIdleTimeout();

  void OnAsrSessionDisconnected();
  void TearDown();

  CreateBackgroundWebContentsCallback create_background_web_contents_;

  State state_ = State::kIdle;

  // The guest profile's primary OTR profile, which hosts the worker's
  // BackgroundWebContents and is the profile we observe for destruction.
  raw_ptr<Profile> otr_profile_ = nullptr;
  base::ScopedObservation<Profile, ProfileObserver> profile_observation_{this};

  std::unique_ptr<local_ai::BackgroundWebContents> background_web_contents_;

  mojo::ReceiverSet<local_ai::mojom::AsrSession> asr_session_receivers_;

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

  // Tears the worker down if it never registers its factory. Cancelled in
  // RegisterFactory(); crashes past that point are caught by factory_'s
  // disconnect handler.
  base::OneShotTimer startup_timer_;
  static constexpr base::TimeDelta kStartupTimeout = base::Seconds(30);

  // Tears the worker down after it has had no active AsrSession for
  // kIdleTimeout. Armed when the last session disconnects
  // (OnAsrSessionDisconnected); cancelled in GetAsrSession() when a new
  // session is handed out. The fired timer re-checks for sessions before
  // tearing down.
  base::OneShotTimer idle_timer_;
  static constexpr base::TimeDelta kIdleTimeout = base::Seconds(60);

  base::WeakPtrFactory<OnDeviceSpeechRecognitionController> weak_factory_{this};
};

}  // namespace speech

#endif  // BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_CONTROLLER_H_
