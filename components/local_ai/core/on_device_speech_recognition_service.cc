// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/core/on_device_speech_recognition_service.h"

#include <utility>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/file_util.h"
#include "mojo/public/cpp/base/big_buffer.h"

namespace local_ai {

namespace {

// TODO(jocelyn): Replace hard-coded path with component
// updater integration.
constexpr char kDefaultModelDir[] =
    "/Users/yrliou/brave/brave-browser/src/brave/"
    "components/local_ai/resources/candle_whisper/"
    "tmp/whisper-tiny";

mojom::SpeechRecognitionModelFilesPtr LoadSpeechModelFilesFromDisk(
    const base::FilePath& dir) {
  auto weights = ReadFileToBigBuffer(dir.AppendASCII("model.safetensors"));
  if (!weights) {
    return nullptr;
  }

  auto tokenizer = ReadFileToBigBuffer(dir.AppendASCII("tokenizer.json"));
  if (!tokenizer) {
    return nullptr;
  }

  auto mel_filters = ReadFileToBigBuffer(dir.AppendASCII("mel_filters.bytes"));
  if (!mel_filters) {
    return nullptr;
  }

  auto config = ReadFileToBigBuffer(dir.AppendASCII("config.json"));
  if (!config) {
    return nullptr;
  }

  auto files = mojom::SpeechRecognitionModelFiles::New();
  files->weights = std::move(*weights);
  files->tokenizer = std::move(*tokenizer);
  files->mel_filters = std::move(*mel_filters);
  files->config = std::move(*config);
  return files;
}

}  // namespace

OnDeviceSpeechRecognitionService::OnDeviceSpeechRecognitionService(
    BackgroundWebContentsFactory factory)
    : OnDeviceSpeechRecognitionService(std::move(factory),
                                       base::FilePath(kDefaultModelDir)) {}

OnDeviceSpeechRecognitionService::OnDeviceSpeechRecognitionService(
    BackgroundWebContentsFactory factory,
    const base::FilePath& model_dir)
    : LocalAIServiceBase(std::move(factory),
                         /*models_already_available=*/true),
      model_dir_(model_dir) {
  CHECK(
      base::FeatureList::IsEnabled(features::kBraveOnDeviceSpeechRecognition));
  DVLOG(3) << "OnDeviceSpeechRecognitionService created";
}

OnDeviceSpeechRecognitionService::~OnDeviceSpeechRecognitionService() = default;

mojo::PendingRemote<mojom::OnDeviceSpeechRecognitionService>
OnDeviceSpeechRecognitionService::MakeRemote() {
  mojo::PendingRemote<mojom::OnDeviceSpeechRecognitionService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void OnDeviceSpeechRecognitionService::Bind(
    mojo::PendingReceiver<mojom::OnDeviceSpeechRecognitionService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void OnDeviceSpeechRecognitionService::RegisterSpeechRecognitionFactory(
    mojo::PendingRemote<mojom::SpeechRecognitionFactory> factory) {
  factory_.Bind(std::move(factory));
  HandleFactoryRegistered();
}

void OnDeviceSpeechRecognitionService::CreateSession(
    on_device_model::mojom::AsrStreamOptionsPtr options,
    mojo::PendingReceiver<on_device_model::mojom::AsrStreamInput> stream,
    mojo::PendingRemote<on_device_model::mojom::AsrStreamResponder> responder,
    CreateSessionCallback callback) {
  MaybeCreateBWC();
  if (ReadyToServe()) {
    ForwardSession(std::move(options), std::move(stream),
                   std::move(responder), std::move(callback));
    return;
  }
  // Split callback so on_cancel can run it if load fails.
  auto [on_ready_cb, on_cancel_cb] =
      base::SplitOnceCallback(std::move(callback));
  QueueConsumer(
      base::BindOnce(&OnDeviceSpeechRecognitionService::ForwardSession,
                     weak_ptr_factory_.GetWeakPtr(), std::move(options),
                     std::move(stream), std::move(responder),
                     std::move(on_ready_cb)),
      std::move(on_cancel_cb));
}

void OnDeviceSpeechRecognitionService::NotifySpeechRecognitionIdle() {
  DVLOG(3) << "OnDeviceSpeechRecognitionService: "
              "All sessions disconnected";
  NotifyIdle();
}

void OnDeviceSpeechRecognitionService::LoadModelFiles(
    base::OnceCallback<void(bool)> on_done) {
  DVLOG(1) << "OnDeviceSpeechRecognitionService: "
              "Loading model files from "
           << model_dir_;

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&LoadSpeechModelFilesFromDisk, model_dir_),
      base::BindOnce(&OnDeviceSpeechRecognitionService::OnFilesLoaded,
                     weak_ptr_factory_.GetWeakPtr(), std::move(on_done)));
}

void OnDeviceSpeechRecognitionService::OnFilesLoaded(
    base::OnceCallback<void(bool)> on_done,
    mojom::SpeechRecognitionModelFilesPtr model_files) {
  DVLOG(3) << "OnDeviceSpeechRecognitionService::"
              "OnFilesLoaded";
  loaded_model_files_ = std::move(model_files);
  std::move(on_done).Run(!!loaded_model_files_);
}

void OnDeviceSpeechRecognitionService::InitModelViaFactory(
    base::OnceCallback<void(bool)> on_complete) {
  factory_->Init(std::move(loaded_model_files_), std::move(on_complete));
}

bool OnDeviceSpeechRecognitionService::IsFactoryBound() const {
  return factory_.is_bound();
}

void OnDeviceSpeechRecognitionService::ResetFactory() {
  factory_.reset();
}

void OnDeviceSpeechRecognitionService::SetFactoryDisconnectHandler(
    base::OnceClosure handler) {
  factory_.set_disconnect_handler(std::move(handler));
}

void OnDeviceSpeechRecognitionService::OnShutdownExtra() {
  receivers_.Clear();
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void OnDeviceSpeechRecognitionService::ForwardSession(
    on_device_model::mojom::AsrStreamOptionsPtr options,
    mojo::PendingReceiver<on_device_model::mojom::AsrStreamInput> stream,
    mojo::PendingRemote<on_device_model::mojom::AsrStreamResponder> responder,
    CreateSessionCallback callback) {
  factory_->CreateSession(std::move(options), std::move(stream),
                          std::move(responder));
  std::move(callback).Run();
}

}  // namespace local_ai
