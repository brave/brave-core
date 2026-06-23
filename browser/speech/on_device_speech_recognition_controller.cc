/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/on_device_speech_recognition_controller.h"

#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "base/no_destructor.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/local_ai/background_web_contents_factory.h"
#include "brave/components/local_ai/core/url_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "services/network/public/mojom/web_sandbox_flags.mojom-shared.h"
#include "url/gurl.h"

namespace speech {

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
OnDeviceSpeechRecognitionController::CreateForTesting(
    CreateBackgroundWebContentsCallback create_background_web_contents) {
  return base::WrapUnique(new OnDeviceSpeechRecognitionController(
      std::move(create_background_web_contents)));
}

OnDeviceSpeechRecognitionController::OnDeviceSpeechRecognitionController(
    CreateBackgroundWebContentsCallback create_background_web_contents)
    : create_background_web_contents_(
          std::move(create_background_web_contents)) {}

OnDeviceSpeechRecognitionController::~OnDeviceSpeechRecognitionController() =
    default;

void OnDeviceSpeechRecognitionController::OnBackgroundContentsDestroyed(
    local_ai::BackgroundWebContents::DestroyReason reason) {
  // This is invoked from within the BackgroundWebContents'
  // WebContentsObserver callback. TearDown() destroys the
  // BackgroundWebContents (and its WebContents), which must not happen
  // synchronously inside an observer notification (it trips
  // WebContentsImpl's is_notifying_observers() CHECK). Defer it.
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
  // The created reply is free-floating (owned by the guest-profile async
  // creation), so it is bound weakly.
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
  TearDown();
}

void OnDeviceSpeechRecognitionController::TearDown() {
  idle_timer_.Stop();
  background_web_contents_.reset();
  profile_observation_.Reset();
  otr_profile_ = nullptr;
  state_ = State::kIdle;
}

}  // namespace speech
