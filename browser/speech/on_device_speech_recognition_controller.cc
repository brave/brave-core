/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/on_device_speech_recognition_controller.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/local_ai/content/background_web_contents_impl.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/url_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/task_manager/web_contents_tags.h"
#include "services/network/public/cpp/web_sandbox_flags.h"
#include "url/gurl.h"

namespace speech {

// static
OnDeviceSpeechRecognitionController*
OnDeviceSpeechRecognitionController::Get() {
  static base::NoDestructor<OnDeviceSpeechRecognitionController> instance;
  return instance.get();
}

OnDeviceSpeechRecognitionController::OnDeviceSpeechRecognitionController() {}

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
  EnsureBackgroundContents();
}

void OnDeviceSpeechRecognitionController::EnsureBackgroundContents() {
  auto* profile_manager = g_browser_process->profile_manager();
  CHECK(profile_manager);
  profile_manager->CreateProfileAsync(
      ProfileManager::GetGuestProfilePath(),
      base::BindOnce(
          &OnDeviceSpeechRecognitionController::OnGuestProfileCreated,
          weak_factory_.GetWeakPtr()));
}

void OnDeviceSpeechRecognitionController::OnGuestProfileCreated(
    Profile* guest_profile) {
  if (!guest_profile) {
    TearDown();
    return;
  }
  // Host the worker in the guest profile's primary OTR profile so the
  // renderer's storage is ephemeral and, on shutdown, is destroyed before
  // its parent. We observe the OTR profile and tear down on its destruction
  // so the WebContents never outlives its BrowserContext (which would trip
  // BrowserContextImpl's rph_with_bc_reference NOTREACHED).
  Profile* otr_profile =
      guest_profile->GetPrimaryOTRProfile(/*create_if_needed=*/true);
  otr_profile_ = otr_profile;
  profile_observation_.Observe(otr_profile);

  // The ORT worker must be cross-origin isolated (COOP: same-origin +
  // COEP: require-corp) so onnxruntime-web can use SharedArrayBuffer for
  // multi-threaded WASM. Chromium blocks navigating a COOP (non
  // unsafe-none) document that has ANY sandbox flag set — see
  // CrossOriginOpenerPolicyStatus::SanitizeResponse,
  // content/browser/security/coop/cross_origin_opener_policy_status.cc
  // (returns kCoopSandboxedIFrameCannotNavigateToCoopPage when
  // sandbox_flags != kNone). The check is binary (any flag triggers it),
  // so a cross-origin-isolated worker MUST be fully unsandboxed; there is
  // no narrower subset. It still lives in an isolated chrome-untrusted
  // renderer process, which is where the real containment comes from.
  background_web_contents_ =
      std::make_unique<local_ai::BackgroundWebContentsImpl>(
          otr_profile, GURL(local_ai::kOnDeviceSpeechRecognitionOrtWorkerURL),
          this, base::BindOnce([](content::WebContents* web_contents) {
            task_manager::WebContentsTags::CreateForToolContents(
                web_contents,
                IDS_ON_DEVICE_SPEECH_RECOGNITION_TASK_MANAGER_TITLE);
          }),
          network::mojom::WebSandboxFlags::kNone);
}

void OnDeviceSpeechRecognitionController::StartIdleTimer() {
  idle_timer_.Start(
      FROM_HERE, kIdleTimeout,
      base::BindOnce(&OnDeviceSpeechRecognitionController::OnIdleTimeout,
                     weak_factory_.GetWeakPtr()));
}

void OnDeviceSpeechRecognitionController::OnIdleTimeout() {
  TearDown();
}

void OnDeviceSpeechRecognitionController::OnFactoryDisconnected() {
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
