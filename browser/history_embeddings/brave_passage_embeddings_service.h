// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_H_

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/local_ai/core/background_web_contents.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "brave/components/local_ai/core/local_models_updater.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/passage_embeddings/public/mojom/passage_embeddings.mojom.h"

namespace content {
class WebContents;
}

namespace passage_embeddings {

// In-process implementation of passage_embeddings::mojom::PassageEmbedder
// that wraps a renderer-side local_ai PassageEmbedder. Upstream's
// SchedulingEmbedder speaks this mojom; we translate its batch calls to
// the renderer's single-passage interface, processing passages
// sequentially so callbacks resolve with all embeddings in order.
class BraveBatchPassageEmbedder : public mojom::PassageEmbedder {
 public:
  BraveBatchPassageEmbedder(
      mojo::PendingReceiver<mojom::PassageEmbedder> receiver,
      mojo::PendingRemote<local_ai::mojom::PassageEmbedder> renderer_embedder,
      base::OnceClosure on_disconnect);
  ~BraveBatchPassageEmbedder() override;

  BraveBatchPassageEmbedder(const BraveBatchPassageEmbedder&) = delete;
  BraveBatchPassageEmbedder& operator=(const BraveBatchPassageEmbedder&) =
      delete;

  // mojom::PassageEmbedder:
  void GenerateEmbeddings(const std::vector<std::string>& passages,
                          mojom::PassagePriority priority,
                          GenerateEmbeddingsCallback callback) override;

 private:
  struct Batch {
    Batch();
    ~Batch();
    Batch(Batch&&) noexcept;
    Batch& operator=(Batch&&) noexcept;

    std::vector<std::string> passages;
    std::vector<mojom::PassageEmbeddingsResultPtr> results;
    GenerateEmbeddingsCallback callback;
    size_t next_index = 0;
  };

  void ProcessNext();
  void OnOneEmbedding(const std::vector<double>& embedding);
  void FailCurrentBatch();
  void OnDisconnected();

  static std::optional<std::vector<float>> ToFloatEmbedding(
      const std::vector<double>& embedding);

  std::optional<Batch> current_batch_;
  std::deque<Batch> pending_batches_;

  mojo::Receiver<mojom::PassageEmbedder> receiver_;
  mojo::Remote<local_ai::mojom::PassageEmbedder> renderer_embedder_;
  base::OnceClosure on_disconnect_;

  base::WeakPtrFactory<BraveBatchPassageEmbedder> weak_ptr_factory_{this};
};

// In-process implementation of
// passage_embeddings::mojom::PassageEmbeddingsService. Takes the place of
// the utility-process PassageEmbeddingsService that upstream Chrome
// launches — the PassageEmbeddingsServiceController binds its
// service_remote_ directly to an instance of this class, so no IPC hop
// is involved.
//
// Owns:
//   - The guest-OTR BackgroundWebContents that hosts the WASM worker.
//   - The renderer-registered PassageEmbedderFactory remote.
//   - A BraveBatchPassageEmbedder that implements the upstream mojom
//     PassageEmbedder by fanning batches to the renderer.
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
  // — binds `receiver` to our BraveBatchPassageEmbedder and invokes
  // `callback` once the WASM renderer is ready. The upstream mojom is
  // shaped for a tflite + sentencepiece embedder in a sandboxed utility
  // process; we run in-process with a five-file EmbeddingGemma model, so
  // the struct doesn't fit and the mojo hop adds no isolation. The
  // controller calls this directly and leaves service_remote_ unbound.
  // Model files are delivered separately via PassageEmbedderFactory::Init
  // as local_ai::mojom::ModelFiles BigBuffers. See README.md for details.
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
    LoadModelsCallback callback;
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
  // Arms a fresh barrier that waits for two events before triggering
  // LoadLocalModelFiles: (1) the EmbeddingGemma component is installed
  // (OnLocalModelsReady fires, or install_dir is already non-empty
  // at arm time) and (2) the renderer's PassageEmbedderFactory
  // registers via RegisterPassageEmbedderFactory.
  void MaybeWaitForLocalModelFilesReady();
  // Counts the "component ready" half of the barrier exactly once per
  // barrier lifetime. Both MaybeWaitForLocalModelFilesReady's sync
  // check AND OnLocalModelsReady route through here, so AddObserver's
  // immediate-notify (which fires OnLocalModelsReady synchronously
  // when the component was already installed at AddObserver time)
  // doesn't double-count the same signal.
  void CountComponentReadyOnce();
  void LoadLocalModelFiles();
  void OnLocalModelFilesLoaded(local_ai::mojom::ModelFilesPtr model_files);
  void OnFactoryInitDone(bool success);
  void FulfillPendingLoads();
  void FailPendingLoads();
  // Drops the renderer-facing embedder state (batch embedder, factory,
  // pending loads, model-ready flag) and re-arms the load barrier.
  // Shared by OnFactoryDisconnected and CloseBackgroundContents.
  void ResetEmbedderState();
  void OnFactoryDisconnected();
  void OnBatchEmbedderDisconnected();

  std::unique_ptr<local_ai::BackgroundWebContents> background_web_contents_;
  BackgroundWebContentsFactory background_web_contents_factory_;
  raw_ptr<local_ai::LocalModelsUpdaterState> updater_state_;

  mojo::ReceiverSet<local_ai::mojom::LocalAIService> local_ai_receivers_;
  mojo::Remote<local_ai::mojom::PassageEmbedderFactory> factory_;

  std::unique_ptr<BraveBatchPassageEmbedder> batch_embedder_;
  std::vector<PendingLoad> pending_loads_;

  // BarrierClosure that fires LoadLocalModelFiles once both halves
  // (component-ready + factory-registered) have contributed their
  // count. Re-armed on each arm cycle by MaybeWaitForLocalModelFilesReady.
  base::RepeatingClosure model_load_barrier_;
  // Prevents AddObserver's immediate OnLocalModelsReady notification
  // from counting install_dir twice for the same barrier — once from
  // MaybeWaitForLocalModelFilesReady's sync check and once from the
  // observer callback.
  bool component_ready_counted_ = false;
  bool model_ready_ = false;

  base::WeakPtrFactory<BravePassageEmbeddingsService> weak_ptr_factory_{this};
};

}  // namespace passage_embeddings

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_H_
