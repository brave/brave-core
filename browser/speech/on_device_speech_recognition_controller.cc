/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/on_device_speech_recognition_controller.h"

#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "base/no_destructor.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/local_ai/background_web_contents_factory.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/url_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "services/network/public/mojom/web_sandbox_flags.mojom-shared.h"
#include "url/gurl.h"

namespace speech {

OnDeviceSpeechRecognitionController::PendingSession::PendingSession() = default;
OnDeviceSpeechRecognitionController::PendingSession::PendingSession(
    PendingSession&&) = default;
OnDeviceSpeechRecognitionController::PendingSession&
OnDeviceSpeechRecognitionController::PendingSession::operator=(
    PendingSession&&) = default;
OnDeviceSpeechRecognitionController::PendingSession::~PendingSession() =
    default;

// static
OnDeviceSpeechRecognitionController*
OnDeviceSpeechRecognitionController::Get() {
  // The speech worker must be cross-origin isolated (COOP: same-origin +
  // COEP: require-corp) so onnxruntime-web can use SharedArrayBuffer for
  // multi-threaded WASM. Chromium blocks navigating a COOP (non unsafe-none)
  // document that has ANY sandbox flag set — see
  // CrossOriginOpenerPolicyStatus::SanitizeResponse,
  // content/browser/security/coop/cross_origin_opener_policy_status.cc
  // (returns kCoopSandboxedIFrameCannotNavigateToCoopPage when
  // sandbox_flags != kNone). The check is binary (any flag triggers it), so a
  // cross-origin-isolated worker MUST be fully unsandboxed; there is no
  // narrower subset. It still lives in an isolated chrome-untrusted renderer
  // process, which is where the real containment comes from.
  static base::NoDestructor<OnDeviceSpeechRecognitionController> instance(
      base::BindRepeating(&local_ai::CreateBackgroundWebContents,
                          GURL(local_ai::kOnDeviceSpeechRecognitionWorkerURL),
                          IDS_ON_DEVICE_SPEECH_RECOGNITION_TASK_MANAGER_TITLE,
                          network::mojom::WebSandboxFlags::kNone));
  return instance.get();
}

// static
std::unique_ptr<OnDeviceSpeechRecognitionController>
OnDeviceSpeechRecognitionController::CreateForTesting(  // IN-TEST
    CreateBackgroundWebContentsCallback create_background_web_contents) {
  return base::WrapUnique(new OnDeviceSpeechRecognitionController(
      std::move(create_background_web_contents)));
}

OnDeviceSpeechRecognitionController::OnDeviceSpeechRecognitionController(
    CreateBackgroundWebContentsCallback create_background_web_contents)
    : create_background_web_contents_(
          std::move(create_background_web_contents)) {
  asr_session_receivers_.set_disconnect_handler(base::BindRepeating(
      &OnDeviceSpeechRecognitionController::OnAsrSessionDisconnected,
      base::Unretained(this)));
}

OnDeviceSpeechRecognitionController::~OnDeviceSpeechRecognitionController() =
    default;

mojo::PendingRemote<local_ai::mojom::AsrSession>
OnDeviceSpeechRecognitionController::GetAsrSession() {
  mojo::PendingRemote<local_ai::mojom::AsrSession> remote;
  asr_session_receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  idle_timer_.Stop();
  return remote;
}

void OnDeviceSpeechRecognitionController::Start(
    on_device_model::mojom::AsrStreamOptionsPtr options,
    mojo::PendingReceiver<on_device_model::mojom::AsrStreamInput> stream,
    mojo::PendingRemote<on_device_model::mojom::AsrStreamResponder> responder) {
  // No model installed: don't spin up a guest profile + cross-origin-isolated
  // renderer just to discover the files are missing. Dropping the
  // stream/responder pipes here surfaces to the engine as a recognition
  // failure, the same outcome as a failed load.
  if (local_ai::OnDeviceSpeechModelsState::GetInstance()
          ->GetModelDir()
          .empty()) {
    return;
  }

  // Worker is not ready yet, start the worker if not already starting.
  if (state_ == State::kIdle) {
    StartWorker();
  }

  // Queue the session until the worker is ready.
  PendingSession pending;
  pending.options = std::move(options);
  pending.stream = std::move(stream);
  pending.responder = std::move(responder);
  pending_sessions_.push_back(std::move(pending));
}

void OnDeviceSpeechRecognitionController::OnAsrSessionDisconnected() {
  // A consumer dropped its AsrSession remote, so that session ended. When the
  // last one goes, arm the idle timer to tear the worker down.
  if (asr_session_receivers_.empty()) {
    StartIdleTimer();
  }
}

void OnDeviceSpeechRecognitionController::OnBackgroundContentsDestroyed(
    local_ai::BackgroundWebContents::DestroyReason reason) {
  // Invoked from within BackgroundWebContents::NotifyDestroyed, which deletes
  // the BackgroundWebContents immediately after this returns. Calling
  // TearDown() synchronously would reset() that same BackgroundWebContents
  // from inside its own call stack and destroy its WebContents while it is
  // still notifying observers (trips WebContentsImpl's is_notifying_observers()
  // CHECK). Defer it.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&OnDeviceSpeechRecognitionController::TearDown,
                                weak_factory_.GetWeakPtr()));
}

void OnDeviceSpeechRecognitionController::OnProfileWillBeDestroyed(
    Profile* profile) {
  if (profile != otr_profile_) {
    return;
  }
  TearDown();
}

void OnDeviceSpeechRecognitionController::StartWorker() {
  if (state_ != State::kIdle) {
    return;
  }
  state_ = State::kBwcStarting;

  // startup_timer_ is a member, so it cannot fire after `this` is destroyed.
  startup_timer_.Start(
      FROM_HERE, kStartupTimeout,
      base::BindOnce(&OnDeviceSpeechRecognitionController::OnStartupTimeout,
                     base::Unretained(this)));

  // Worker creation is asynchronous and owned by the profile/WebContents
  // creation pipeline, not by this controller, so it cannot be canceled once
  // started. Bind the delegate and the reply weakly so that if TearDown() runs
  // before creation finishes, InvalidateWeakPtrs() drops this reply and it
  // cannot run against a later worker cycle.
  create_background_web_contents_.Run(
      weak_factory_.GetWeakPtr(),
      base::BindOnce(
          &OnDeviceSpeechRecognitionController::OnBackgroundContentsCreated,
          weak_factory_.GetWeakPtr()));
}

void OnDeviceSpeechRecognitionController::OnBackgroundContentsCreated(
    std::unique_ptr<local_ai::BackgroundWebContents> background_web_contents,
    Profile* otr_profile) {
  if (state_ != State::kBwcStarting) {
    // Torn down while the worker environment was being created.
    return;
  }

  if (!background_web_contents || !otr_profile) {
    TearDown();
    return;
  }

  background_web_contents_ = std::move(background_web_contents);
  otr_profile_ = otr_profile;

  // Observe the OTR profile the worker lives in and tear down on its
  // destruction so the WebContents never outlives its BrowserContext (which
  // would trip BrowserContextImpl's rph_with_bc_reference NOTREACHED).
  profile_observation_.Observe(otr_profile);

  state_ = State::kWorkerStarting;
}

void OnDeviceSpeechRecognitionController::StartIdleTimer() {
  // idle_timer_ is a member, so it cannot fire after `this` is destroyed.
  idle_timer_.Start(
      FROM_HERE, kIdleTimeout,
      base::BindOnce(&OnDeviceSpeechRecognitionController::OnIdleTimeout,
                     base::Unretained(this)));
}

void OnDeviceSpeechRecognitionController::OnIdleTimeout() {
  if (!asr_session_receivers_.empty()) {
    return;
  }
  TearDown();
}

void OnDeviceSpeechRecognitionController::OnStartupTimeout() {
  if (state_ != State::kBwcStarting && state_ != State::kWorkerStarting) {
    return;
  }
  TearDown();
}

void OnDeviceSpeechRecognitionController::TearDown() {
  // Drop outstanding weak-bound replies (BWC creation, deferred teardown) so
  // none from this cycle can land in a later one. The singleton is never
  // destroyed, so nothing else invalidates them.
  weak_factory_.InvalidateWeakPtrs();
  startup_timer_.Stop();
  idle_timer_.Stop();
  background_web_contents_.reset();
  profile_observation_.Reset();
  otr_profile_ = nullptr;
  pending_sessions_.clear();
  asr_session_receivers_.Clear();
  state_ = State::kIdle;
}

}  // namespace speech
