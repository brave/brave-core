// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_H_

#include <memory>
#include <optional>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/one_shot_event.h"
#include "brave/browser/history_embeddings/brave_batch_passage_embedder.h"
#include "brave/components/local_ai/core/background_web_contents.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "brave/components/local_ai/core/local_models_updater.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/passage_embeddings/public/mojom/passage_embeddings.mojom.h"

namespace content {
class WebContents;
}

namespace passage_embeddings {

// In-process implementation of
// passage_embeddings::mojom::PassageEmbeddingsService. Takes the place of
// the utility-process PassageEmbeddingsService that upstream Chrome
// launches — the PassageEmbeddingsServiceController binds its
// service_remote_ directly to an instance of this class, so no IPC hop
// is involved.
//
// Matches the shape of upstream's PassageEmbeddingsService: on
// LoadModels we create a PassageEmbedder (here,
// BraveBatchPassageEmbedder) and hand it the loaded model files plus
// the factory remote. All init/bind/disconnect bookkeeping lives
// inside the BatchEmbedder, which calls us back via on_disconnect when
// it is torn down.
//
// This service still owns:
//   - The guest-OTR BackgroundWebContents that hosts the WASM worker.
//   - The OneShotEvents that gate file loading on the EmbeddingGemma
//     component install and the renderer registering its factory.
//   - The PassageEmbedderFactory remote received via
//     RegisterPassageEmbedderFactory (until it is moved into the
//     BatchEmbedder on successful load).
//
// Also implements local_ai::mojom::LocalAIService so the renderer WASM
// page can register its PassageEmbedderFactory back to us via
// UntrustedLocalAIUI::BindInterface.
class BravePassageEmbeddingsService
    : public mojom::PassageEmbeddingsService,
      public local_ai::mojom::LocalAIService,
      public local_ai::BackgroundWebContents::Delegate,
      public local_ai::LocalModelsUpdaterState::Observer {
 public:
  using BackgroundWebContentsCreatedCallback = base::OnceCallback<void(
      std::unique_ptr<local_ai::BackgroundWebContents>)>;
  using BackgroundWebContentsFactory = base::RepeatingCallback<void(
      local_ai::BackgroundWebContents::Delegate* delegate,
      BackgroundWebContentsCreatedCallback)>;

  BravePassageEmbeddingsService(
      BackgroundWebContentsFactory background_web_contents_factory,
      local_ai::LocalModelsUpdaterState* updater_state);
  ~BravePassageEmbeddingsService() override;

  BravePassageEmbeddingsService(const BravePassageEmbeddingsService&) = delete;
  BravePassageEmbeddingsService& operator=(
      const BravePassageEmbeddingsService&) = delete;

  // Registry for routing mojo binding requests from the background
  // WebContents (on guest OTR) back to the active service instance.
  // UntrustedLocalAIUI::BindInterface calls BindForWebContents; the
  // service installs SetBindCallbackForWebContents when it creates its
  // background contents.
  using BindCallback = base::RepeatingCallback<void(
      mojo::PendingReceiver<local_ai::mojom::LocalAIService>)>;
  static void SetBindCallbackForWebContents(content::WebContents* web_contents,
                                            BindCallback callback);
  static void RemoveBindCallbackForWebContents(
      content::WebContents* web_contents);
  static void BindForWebContents(
      content::WebContents* web_contents,
      mojo::PendingReceiver<local_ai::mojom::LocalAIService> receiver);

  // Binds a renderer-facing receiver. Installed as the BindCallback by
  // the controller when it creates the background contents.
  void BindLocalAIReceiver(
      mojo::PendingReceiver<local_ai::mojom::LocalAIService> receiver);

  base::WeakPtr<BravePassageEmbeddingsService> GetWeakPtr();

  // Direct in-process equivalent of mojom::PassageEmbeddingsService::LoadModels
  // — queues the receiver + callback as the pending load and drives
  // the startup flow. The upstream mojom is shaped for a tflite +
  // sentencepiece embedder in a sandboxed utility process; we run
  // in-process with a five-file EmbeddingGemma model, so the struct
  // doesn't fit and the mojo hop adds no isolation. The controller
  // calls this directly and leaves service_remote_ unbound. Model
  // files are delivered separately via PassageEmbedderFactory::Init as
  // local_ai::mojom::ModelFiles BigBuffers. See README.md for details.
  void BindPassageEmbedder(
      mojo::PendingReceiver<mojom::PassageEmbedder> receiver,
      base::OnceCallback<void(bool)> callback);

  // mojom::PassageEmbeddingsService:
  void LoadModels(mojom::PassageEmbeddingsLoadModelsParamsPtr model_params,
                  mojom::PassageEmbedderParamsPtr params,
                  mojo::PendingReceiver<mojom::PassageEmbedder> model,
                  LoadModelsCallback callback) override;

  // local_ai::mojom::LocalAIService:
  void RegisterPassageEmbedderFactory(
      mojo::PendingRemote<local_ai::mojom::PassageEmbedderFactory> factory)
      override;

 private:
  struct PendingLoad {
    PendingLoad();
    ~PendingLoad();
    PendingLoad(PendingLoad&&) noexcept;
    PendingLoad& operator=(PendingLoad&&) noexcept;

    mojo::PendingReceiver<mojom::PassageEmbedder> receiver;
    base::OnceCallback<void(bool)> callback;
  };

  // LocalModelsUpdaterState::Observer:
  void OnLocalModelsReady(const base::FilePath& install_dir) override;

  // BackgroundWebContents::Delegate:
  void OnBackgroundContentsDestroyed(
      local_ai::BackgroundWebContents::DestroyReason reason) override;

  void MaybeCreateBackgroundContents();
  void OnBackgroundContentsCreated(
      std::unique_ptr<local_ai::BackgroundWebContents> contents);
  void CloseBackgroundContents();
  // Arms a fresh pair of OneShotEvents. Signal() on each is idempotent
  // under is_signaled() — AddObserver's sync re-fire of
  // OnLocalModelsReady cannot double-count.
  void MaybeWaitForLocalModelFilesReady();
  // Invoked from BindPassageEmbedder, RegisterPassageEmbedderFactory,
  // and OnLocalModelsReady. Proceeds to LoadLocalModelFiles only when
  // (1) both OneShotEvents have signaled and (2) a pending load is
  // queued and (3) no BatchEmbedder is already active.
  void MaybeStartLoad();
  void LoadLocalModelFiles();
  void OnLocalModelFilesLoaded(local_ai::mojom::ModelFilesPtr model_files);
  void OnBatchEmbedderDisconnected();
  // Handler installed on factory_ while the remote is held by the
  // service (before it is moved into the BatchEmbedder). Once the
  // BatchEmbedder owns the factory it installs its own disconnect
  // handler.
  void OnEarlyFactoryDisconnected();
  // Drops batch_embedder_/factory_, fails any pending load, re-arms
  // the load events, and invalidates in-flight load callbacks.
  void ResetEmbedderState();

  std::unique_ptr<local_ai::BackgroundWebContents> background_web_contents_;
  // Set while the factory callback is in flight so successive
  // MaybeCreateBackgroundContents calls don't kick off duplicate
  // creations before OnBackgroundContentsCreated runs.
  bool background_web_contents_creating_ = false;
  BackgroundWebContentsFactory background_web_contents_factory_;
  raw_ptr<local_ai::LocalModelsUpdaterState> updater_state_;

  mojo::ReceiverSet<local_ai::mojom::LocalAIService> local_ai_receivers_;
  // Holds the renderer's factory remote from RegisterPassageEmbedderFactory
  // until a successful load moves it into the BatchEmbedder.
  mojo::Remote<local_ai::mojom::PassageEmbedderFactory> factory_;

  std::unique_ptr<BraveBatchPassageEmbedder> batch_embedder_;
  std::optional<PendingLoad> pending_load_;

  // Signaled when the EmbeddingGemma component is installed. Re-armed
  // (new OneShotEvent instance) on each arm cycle by
  // MaybeWaitForLocalModelFilesReady; Signal() is idempotent via the
  // is_signaled() guard at each call site.
  std::unique_ptr<base::OneShotEvent> component_ready_event_;
  // Signaled when the renderer's PassageEmbedderFactory registers.
  std::unique_ptr<base::OneShotEvent> factory_registered_event_;

  // Separate factory for the ThreadPool file-load reply so
  // ResetEmbedderState can cancel an in-flight load without also
  // invalidating the rest of the service's weak-bound callbacks.
  base::WeakPtrFactory<BravePassageEmbeddingsService> load_weak_factory_{this};
  base::WeakPtrFactory<BravePassageEmbeddingsService> weak_ptr_factory_{this};
};

}  // namespace passage_embeddings

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_H_
