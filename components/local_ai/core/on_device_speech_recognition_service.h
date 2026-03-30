// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_RECOGNITION_SERVICE_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_RECOGNITION_SERVICE_H_

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/local_ai/core/local_ai_service_base.h"
#include "brave/components/local_ai/core/on_device_speech_recognition.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/on_device_model/public/mojom/on_device_model.mojom.h"

namespace local_ai {

// Manages the lifecycle of a background speech recognition
// WASM worker.
//
// Follows the same push-based model loading pattern as
// LocalAIService: the browser loads model files from disk
// and pushes them to the factory via Init(). Consumers
// obtain recognition sessions via CreateSession().
//
// Flow:
//  1. Consumer calls CreateSession()
//     -> service creates BWC lazily, queues request
//  2. WASM page loads, registers factory
//  3. Barrier fires (models available + factory),
//     service loads model files from disk on thread pool
//  4. Service pushes files to factory via Init()
//  5. Factory creates AsrStreamInput binding
//  6. Consumer receives AsrStreamInput remote
//  7. Consumer calls NotifySpeechRecognitionIdle()
//     -> service closes BWC to free memory
class OnDeviceSpeechRecognitionService
    : public LocalAIServiceBase,
      public mojom::OnDeviceSpeechRecognitionService {
 public:
  explicit OnDeviceSpeechRecognitionService(
      BackgroundWebContentsFactory factory);

  // For testing: custom model directory.
  OnDeviceSpeechRecognitionService(BackgroundWebContentsFactory factory,
                                   const base::FilePath& model_dir);

  ~OnDeviceSpeechRecognitionService() override;

  OnDeviceSpeechRecognitionService(const OnDeviceSpeechRecognitionService&) =
      delete;
  OnDeviceSpeechRecognitionService& operator=(
      const OnDeviceSpeechRecognitionService&) = delete;

  mojo::PendingRemote<mojom::OnDeviceSpeechRecognitionService> MakeRemote();
  void Bind(
      mojo::PendingReceiver<mojom::OnDeviceSpeechRecognitionService> receiver);

  // mojom::OnDeviceSpeechRecognitionService:
  void RegisterSpeechRecognitionFactory(
      mojo::PendingRemote<mojom::SpeechRecognitionFactory> factory) override;
  void CreateSession(
      on_device_model::mojom::AsrStreamOptionsPtr options,
      mojo::PendingReceiver<on_device_model::mojom::AsrStreamInput> stream,
      mojo::PendingRemote<on_device_model::mojom::AsrStreamResponder> responder,
      CreateSessionCallback callback) override;
  void NotifySpeechRecognitionIdle() override;

 private:
  // LocalAIServiceBase:
  bool IsFactoryBound() const override;
  void ResetFactory() override;
  void SetFactoryDisconnectHandler(base::OnceClosure handler) override;
  void LoadModelFiles(base::OnceCallback<void(bool)> on_done) override;
  void InitModelViaFactory(base::OnceCallback<void(bool)> on_complete) override;
  void OnShutdownExtra() override;

  void OnFilesLoaded(base::OnceCallback<void(bool)> on_done,
                     mojom::SpeechRecognitionModelFilesPtr model_files);
  void ForwardSession(
      on_device_model::mojom::AsrStreamOptionsPtr options,
      mojo::PendingReceiver<on_device_model::mojom::AsrStreamInput> stream,
      mojo::PendingRemote<on_device_model::mojom::AsrStreamResponder>
          responder,
      CreateSessionCallback callback);

  mojo::ReceiverSet<mojom::OnDeviceSpeechRecognitionService> receivers_;
  mojo::Remote<mojom::SpeechRecognitionFactory> factory_;
  mojom::SpeechRecognitionModelFilesPtr loaded_model_files_;

  base::FilePath model_dir_;

  base::WeakPtrFactory<OnDeviceSpeechRecognitionService> weak_ptr_factory_{
      this};
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_RECOGNITION_SERVICE_H_
