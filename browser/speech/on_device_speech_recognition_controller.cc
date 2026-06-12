/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/on_device_speech_recognition_controller.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "brave/components/local_ai/content/background_web_contents_impl.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/url_constants.h"
#include "brave/components/local_ai/core/utils.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/task_manager/web_contents_tags.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "services/network/public/cpp/web_sandbox_flags.h"
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

namespace {

// Runs on a ThreadPool blocking sequence. Reads the Nemotron 0.6B streaming
// model's graphs (encoder/decoder), the 128-mel filterbank and the token
// list, all in full, and packs them into an OrtModelFiles. The worker builds
// its ORT sessions directly from these bytes. Only the .onnx + .onnx.data
// (external-data) export is supported as it lets ORT-Web reference the weights
// in place instead of making an extra copy of the model files in WASM memory.
local_ai::mojom::OrtModelFilesPtr ReadNemotronOrtFiles(
    const base::FilePath& model_dir) {
  auto encoder =
      local_ai::ReadFileToBigBuffer(model_dir.AppendASCII("encoder.onnx"));
  if (!encoder) {
    return nullptr;
  }
  auto decoder = local_ai::ReadFileToBigBuffer(
      model_dir.AppendASCII("decoder_joint.onnx"));
  if (!decoder) {
    return nullptr;
  }
  auto encoder_data =
      local_ai::ReadFileToBigBuffer(model_dir.AppendASCII("encoder.onnx.data"));
  if (!encoder_data) {
    return nullptr;
  }
  auto decoder_data = local_ai::ReadFileToBigBuffer(
      model_dir.AppendASCII("decoder_joint.onnx.data"));
  if (!decoder_data) {
    return nullptr;
  }
  auto filterbank =
      local_ai::ReadFileToBigBuffer(model_dir.AppendASCII("filterbank.bin"));
  if (!filterbank) {
    return nullptr;
  }
  auto tokens =
      local_ai::ReadFileToBigBuffer(model_dir.AppendASCII("tokens.txt"));
  if (!tokens) {
    return nullptr;
  }

  auto files = local_ai::mojom::OrtModelFiles::New();
  files->encoder = std::move(*encoder);
  files->decoder = std::move(*decoder);
  files->encoder_data = std::move(*encoder_data);
  files->decoder_data = std::move(*decoder_data);
  files->mel_filters = std::move(*filterbank);
  files->tokens = std::move(*tokens);
  return files;
}

}  // namespace

// static
OnDeviceSpeechRecognitionController*
OnDeviceSpeechRecognitionController::Get() {
  static base::NoDestructor<OnDeviceSpeechRecognitionController> instance;
  return instance.get();
}

OnDeviceSpeechRecognitionController::OnDeviceSpeechRecognitionController() {
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

void OnDeviceSpeechRecognitionController::BindAsrSession(
    mojo::PendingReceiver<local_ai::mojom::AsrSession> receiver) {
  asr_session_receivers_.Add(this, std::move(receiver));
  idle_timer_.Stop();
}

void OnDeviceSpeechRecognitionController::BindFactoryHost(
    mojo::PendingReceiver<local_ai::mojom::SpeechRecognitionFactoryHost>
        receiver) {
  factory_host_receivers_.Add(this, std::move(receiver));
}

void OnDeviceSpeechRecognitionController::RegisterFactory(
    mojo::PendingRemote<local_ai::mojom::SpeechRecognitionFactory> factory) {
  if (state_ != State::kBwcStarting) {
    return;
  }
  factory_.Bind(std::move(factory));
  factory_.set_disconnect_handler(base::BindOnce(
      &OnDeviceSpeechRecognitionController::OnFactoryDisconnected,
      weak_factory_.GetWeakPtr()));
  state_ = State::kModelLoading;
  LoadOrtModel();
}

void OnDeviceSpeechRecognitionController::Start(
    on_device_model::mojom::AsrStreamOptionsPtr options,
    mojo::PendingReceiver<on_device_model::mojom::AsrStreamInput> stream,
    mojo::PendingRemote<on_device_model::mojom::AsrStreamResponder> responder) {
  // No model installed: don't spin up a guest profile + cross-origin-isolated
  // renderer just to discover the files are missing in OnOrtFilesRead.
  // Dropping the stream/responder pipes here surfaces to the engine as a
  // recognition failure, the same outcome as a failed load.
  if (local_ai::OnDeviceSpeechModelsState::GetInstance()
          ->GetInstallDir()
          .empty()) {
    return;
  }

  if (state_ == State::kIdle) {
    StartWorker();
  }

  if (state_ != State::kReady) {
    PendingSession pending;
    pending.options = std::move(options);
    pending.stream = std::move(stream);
    pending.responder = std::move(responder);
    pending_sessions_.push_back(std::move(pending));
    return;
  }

  ForwardSession(std::move(options), std::move(stream), std::move(responder));
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

void OnDeviceSpeechRecognitionController::LoadOrtModel() {
  auto* state = local_ai::OnDeviceSpeechModelsState::GetInstance();
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&ReadNemotronOrtFiles, state->GetModelDir()),
      base::BindOnce(&OnDeviceSpeechRecognitionController::OnOrtFilesRead,
                     weak_factory_.GetWeakPtr()));
}

void OnDeviceSpeechRecognitionController::OnOrtFilesRead(
    local_ai::mojom::OrtModelFilesPtr files) {
  if (!files) {
    VLOG(1)
        << "OnDeviceSpeechRecognition: failed to read ORT model files from "
        << local_ai::OnDeviceSpeechModelsState::GetInstance()->GetModelDir();
    TearDown();
    return;
  }
  factory_->LoadModel(
      std::move(files),
      base::BindOnce(&OnDeviceSpeechRecognitionController::OnOrtInitResult,
                     weak_factory_.GetWeakPtr()));
}

void OnDeviceSpeechRecognitionController::OnOrtInitResult(bool success) {
  if (!success) {
    TearDown();
    return;
  }
  state_ = State::kReady;
  ForwardPendingSessions();
}

void OnDeviceSpeechRecognitionController::ForwardPendingSessions() {
  std::vector<PendingSession> drained;
  drained.swap(pending_sessions_);
  for (auto& pending : drained) {
    ForwardSession(std::move(pending.options), std::move(pending.stream),
                   std::move(pending.responder));
  }
}

void OnDeviceSpeechRecognitionController::ForwardSession(
    on_device_model::mojom::AsrStreamOptionsPtr options,
    mojo::PendingReceiver<on_device_model::mojom::AsrStreamInput> stream,
    mojo::PendingRemote<on_device_model::mojom::AsrStreamResponder> responder) {
  if (!factory_.is_bound()) {
    return;
  }
  factory_->CreateAsrStream(std::move(options), std::move(stream),
                            std::move(responder));
}

void OnDeviceSpeechRecognitionController::StartIdleTimer() {
  idle_timer_.Start(
      FROM_HERE, kIdleTimeout,
      base::BindOnce(&OnDeviceSpeechRecognitionController::OnIdleTimeout,
                     weak_factory_.GetWeakPtr()));
}

void OnDeviceSpeechRecognitionController::OnIdleTimeout() {
  if (!asr_session_receivers_.empty()) {
    return;
  }
  TearDown();
}

void OnDeviceSpeechRecognitionController::OnFactoryDisconnected() {
  TearDown();
}

void OnDeviceSpeechRecognitionController::TearDown() {
  idle_timer_.Stop();
  factory_.reset();
  background_web_contents_.reset();
  profile_observation_.Reset();
  otr_profile_ = nullptr;
  pending_sessions_.clear();
  asr_session_receivers_.Clear();
  factory_host_receivers_.Clear();
  state_ = State::kIdle;
}

}  // namespace speech
