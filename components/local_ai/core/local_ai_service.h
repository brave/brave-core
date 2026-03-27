// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_LOCAL_AI_SERVICE_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_LOCAL_AI_SERVICE_H_

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "brave/components/local_ai/core/local_ai_service_base.h"
#include "brave/components/local_ai/core/local_models_updater.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace local_ai {

// LocalAIService manages the lifecycle of background model workers.
//
// Consumers call GetPassageEmbedder() to obtain a PassageEmbedder remote
// bound directly to the renderer. The service uses a
// PassageEmbedderFactory (registered by the renderer) to create direct
// consumer→renderer bindings on demand — it never proxies inference
// calls itself.
class LocalAIService : public LocalAIServiceBase,
                       public mojom::LocalAIService,
                       public LocalModelsUpdaterState::Observer {
 public:
  LocalAIService(BackgroundWebContentsFactory factory,
                 LocalModelsUpdaterState* updater_state);
  ~LocalAIService() override;

  LocalAIService(const LocalAIService&) = delete;
  LocalAIService& operator=(const LocalAIService&) = delete;

  mojo::PendingRemote<mojom::LocalAIService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::LocalAIService> receiver);

  // mojom::LocalAIService:
  void RegisterPassageEmbedderFactory(
      mojo::PendingRemote<mojom::PassageEmbedderFactory> factory) override;
  void GetPassageEmbedder(GetPassageEmbedderCallback callback) override;
  void NotifyPassageEmbedderIdle() override;

 private:
  // LocalModelsUpdaterState::Observer:
  void OnLocalModelsReady(const base::FilePath& install_dir) override;

  // LocalAIServiceBase:
  bool IsFactoryBound() const override;
  void ResetFactory() override;
  void SetFactoryDisconnectHandler(base::OnceClosure handler) override;
  void LoadModelFiles(base::OnceCallback<void(bool)> on_done) override;
  void InitModelViaFactory(base::OnceCallback<void(bool)> on_complete) override;
  void OnShutdownExtra() override;

  void OnFilesLoaded(base::OnceCallback<void(bool)> on_done,
                     mojom::ModelFilesPtr model_files);
  void BindPassageEmbedder(GetPassageEmbedderCallback callback);

  mojo::ReceiverSet<mojom::LocalAIService> receivers_;
  mojo::Remote<mojom::PassageEmbedderFactory> factory_;
  mojom::ModelFilesPtr loaded_model_files_;

  raw_ptr<LocalModelsUpdaterState> updater_state_;

  base::WeakPtrFactory<LocalAIService> weak_ptr_factory_{this};
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_LOCAL_AI_SERVICE_H_
